#include "cpu_temp_monitor.h"
#include <comdef.h>
#include <iostream>
#include <wbemidl.h>

#pragma comment(lib, "wbemuuid.lib")

CpuTempMonitor::CpuTempMonitor() : comInitialized(false) {
  cachedInfo = {};
  cachedInfo.temperatureC = -1.0;
  cachedInfo.isThrottling = false;
  cachedInfo.available = false;

  // Initialize COM
  HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
  if (SUCCEEDED(hr) || hr == RPC_E_CHANGED_MODE) {
    comInitialized = true;
  }
}

CpuTempMonitor::~CpuTempMonitor() {
  if (comInitialized) {
    // Don't uninitialize COM here - other components may use it
  }
}

double CpuTempMonitor::queryWmiTemperature() {
  if (!comInitialized)
    return -1.0;

  IWbemLocator *pLocator = nullptr;
  IWbemServices *pServices = nullptr;
  IEnumWbemClassObject *pEnumerator = nullptr;
  double temperature = -1.0;

  // Create WMI locator
  HRESULT hr =
      CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER,
                       IID_IWbemLocator, (LPVOID *)&pLocator);
  if (FAILED(hr)) {
    return -1.0;
  }

  // Try root\WMI first (MSAcpi_ThermalZoneTemperature)
  hr = pLocator->ConnectServer(_bstr_t(L"root\\WMI"), nullptr, nullptr, nullptr,
                               0, nullptr, nullptr, &pServices);
  if (SUCCEEDED(hr)) {
    hr = CoSetProxyBlanket(pServices, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
                           nullptr, RPC_C_AUTHN_LEVEL_CALL,
                           RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);

    if (SUCCEEDED(hr)) {
      hr = pServices->ExecQuery(_bstr_t(L"WQL"),
                                _bstr_t(L"SELECT CurrentTemperature FROM "
                                        L"MSAcpi_ThermalZoneTemperature"),
                                WBEM_FLAG_FORWARD_ONLY |
                                    WBEM_FLAG_RETURN_IMMEDIATELY,
                                nullptr, &pEnumerator);

      if (SUCCEEDED(hr) && pEnumerator) {
        IWbemClassObject *pObj = nullptr;
        ULONG uReturn = 0;
        double maxTemp = -1.0;

        while (pEnumerator->Next(500, 1, &pObj, &uReturn) == S_OK) {
          VARIANT vtProp;
          VariantInit(&vtProp);
          hr = pObj->Get(L"CurrentTemperature", 0, &vtProp, nullptr, nullptr);
          if (SUCCEEDED(hr) && vtProp.vt == VT_I4) {
            // Value is in tenths of degrees Kelvin
            double tempC = (static_cast<double>(vtProp.lVal) / 10.0) - 273.15;
            if (tempC > maxTemp && tempC > 0 && tempC < 150) {
              maxTemp = tempC;
            }
          }
          VariantClear(&vtProp);
          pObj->Release();
        }

        if (maxTemp > 0) {
          temperature = maxTemp;
        }
        pEnumerator->Release();
      }
    }
    pServices->Release();
  }

  // Fallback: try OpenHardwareMonitor/LibreHardwareMonitor WMI namespace
  if (temperature < 0) {
    pServices = nullptr;
    hr = pLocator->ConnectServer(_bstr_t(L"root\\OpenHardwareMonitor"), nullptr,
                                 nullptr, nullptr, 0, nullptr, nullptr,
                                 &pServices);

    if (FAILED(hr)) {
      // Try LibreHardwareMonitor namespace
      hr = pLocator->ConnectServer(_bstr_t(L"root\\LibreHardwareMonitor"),
                                   nullptr, nullptr, nullptr, 0, nullptr,
                                   nullptr, &pServices);
    }

    if (SUCCEEDED(hr)) {
      hr = CoSetProxyBlanket(pServices, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
                             nullptr, RPC_C_AUTHN_LEVEL_CALL,
                             RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);

      if (SUCCEEDED(hr)) {
        pEnumerator = nullptr;
        hr = pServices->ExecQuery(
            _bstr_t(L"WQL"),
            _bstr_t(L"SELECT Value FROM Sensor WHERE SensorType='Temperature' "
                    L"AND Name LIKE '%CPU%'"),
            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr,
            &pEnumerator);

        if (SUCCEEDED(hr) && pEnumerator) {
          IWbemClassObject *pObj = nullptr;
          ULONG uReturn = 0;

          if (pEnumerator->Next(500, 1, &pObj, &uReturn) == S_OK) {
            VARIANT vtProp;
            VariantInit(&vtProp);
            hr = pObj->Get(L"Value", 0, &vtProp, nullptr, nullptr);
            if (SUCCEEDED(hr) && vtProp.vt == VT_R4) {
              temperature = static_cast<double>(vtProp.fltVal);
            }
            VariantClear(&vtProp);
            pObj->Release();
          }
          pEnumerator->Release();
        }
      }
      pServices->Release();
    }
  }

  pLocator->Release();
  return temperature;
}

void CpuTempMonitor::update() {
  double temp = queryWmiTemperature();

  if (temp > 0) {
    cachedInfo.temperatureC = temp;
    cachedInfo.isThrottling = (temp > 90.0);
    cachedInfo.available = true;
  } else {
    cachedInfo.available = false;
  }
}

CpuTempInfo CpuTempMonitor::getInfo() const { return cachedInfo; }
