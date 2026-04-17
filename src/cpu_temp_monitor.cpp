#include "cpu_temp_monitor.h"
#include <comdef.h>
#include <iostream>
#include <wbemidl.h>

#pragma comment(lib, "wbemuuid.lib")

CpuTempMonitor::CpuTempMonitor() 
    : comInitialized(false), pLocator(nullptr), pServices(nullptr), pFallbackServices(nullptr) {
  cachedInfo = {};
  cachedInfo.temperatureC = -1.0;
  cachedInfo.isThrottling = false;
  cachedInfo.available = false;

  HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
  if (SUCCEEDED(hr) || hr == RPC_E_CHANGED_MODE) {
    comInitialized = true;
    hr = CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER,
                         IID_IWbemLocator, (LPVOID *)&pLocator);
  }
  
  // Start the background worker
  workerThread = std::thread(&CpuTempMonitor::workerLoop, this);
}

CpuTempMonitor::~CpuTempMonitor() {
  stopWorker = true;
  if (workerThread.joinable()) workerThread.join();
  
  if (pServices) pServices->Release();
  if (pFallbackServices) pFallbackServices->Release();
  if (pLocator) pLocator->Release();
}

void CpuTempMonitor::workerLoop() {
  // Wait a bit for system to settle
  std::this_thread::sleep_for(std::chrono::seconds(1));

  while (!stopWorker) {
    double temp = queryWmiTemperature();
    if (temp <= 0) {
      temp = queryFallbackTemperature();
    }

    {
      std::lock_guard<std::mutex> lock(infoMutex);
      if (temp > 0) {
        cachedInfo.temperatureC = temp;
        cachedInfo.isThrottling = (temp > 90.0);
        cachedInfo.available = true;
      } else {
        cachedInfo.available = false;
      }
    }
    
    // Polling interval
    for (int i = 0; i < 20 && !stopWorker; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }
}

double CpuTempMonitor::queryWmiTemperature() {
  if (!pLocator) return -1.0;
  if (!pServices) {
    HRESULT hr = pLocator->ConnectServer(_bstr_t(L"root\\WMI"), nullptr, nullptr, nullptr,
                                 0, nullptr, nullptr, &pServices);
    if (SUCCEEDED(hr)) {
      CoSetProxyBlanket(pServices, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
                        nullptr, RPC_C_AUTHN_LEVEL_CALL,
                        RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);
    }
  }

  if (!pServices) return -1.0;

  IEnumWbemClassObject *pEnumerator = nullptr;
  double temperature = -1.0;

  HRESULT hr = pServices->ExecQuery(_bstr_t(L"WQL"),
                            _bstr_t(L"SELECT CurrentTemperature FROM MSAcpi_ThermalZoneTemperature"),
                            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                            nullptr, &pEnumerator);

  if (SUCCEEDED(hr) && pEnumerator) {
    IWbemClassObject *pObj = nullptr;
    ULONG uReturn = 0;
    while (pEnumerator->Next(2000, 1, &pObj, &uReturn) == S_OK) {
      VARIANT vtProp;
      VariantInit(&vtProp);
      if (SUCCEEDED(pObj->Get(L"CurrentTemperature", 0, &vtProp, nullptr, nullptr)) && vtProp.vt == VT_I4) {
        double tempC = (static_cast<double>(vtProp.lVal) / 10.0) - 273.15;
        if (tempC > 0 && tempC < 150) temperature = tempC;
      }
      VariantClear(&vtProp);
      pObj->Release();
    }
    pEnumerator->Release();
  }
  return temperature;
}

double CpuTempMonitor::queryFallbackTemperature() {
  if (!pLocator) return -1.0;
  
  if (!pFallbackServices) {
    // Try OHM
    HRESULT hr = pLocator->ConnectServer(_bstr_t(L"root\\OpenHardwareMonitor"), nullptr, nullptr, nullptr, 0, nullptr, nullptr, &pFallbackServices);
    if (FAILED(hr)) {
      // Try LHM
      pLocator->ConnectServer(_bstr_t(L"root\\LibreHardwareMonitor"), nullptr, nullptr, nullptr, 0, nullptr, nullptr, &pFallbackServices);
    }
    if (pFallbackServices) {
      CoSetProxyBlanket(pFallbackServices, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);
    }
  }

  if (!pFallbackServices) return -1.0;

  IEnumWbemClassObject *pEnumerator = nullptr;
  double temperature = -1.0;
  HRESULT hr = pFallbackServices->ExecQuery(_bstr_t(L"WQL"),
                                    _bstr_t(L"SELECT Value FROM Sensor WHERE SensorType='Temperature' AND Name LIKE '%CPU%'"),
                                    WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &pEnumerator);

  if (SUCCEEDED(hr) && pEnumerator) {
    IWbemClassObject *pObj = nullptr;
    ULONG uReturn = 0;
    if (pEnumerator->Next(2000, 1, &pObj, &uReturn) == S_OK) {
      VARIANT vtProp;
      VariantInit(&vtProp);
      if (SUCCEEDED(pObj->Get(L"Value", 0, &vtProp, nullptr, nullptr)) && vtProp.vt == VT_R4) {
        temperature = vtProp.fltVal;
      }
      VariantClear(&vtProp);
      pObj->Release();
    }
    pEnumerator->Release();
  }
  return temperature;
}

void CpuTempMonitor::update() {
    // Logic moved to background worker
}

CpuTempInfo CpuTempMonitor::getInfo() const {
  std::lock_guard<std::mutex> lock(infoMutex);
  return cachedInfo;
}
