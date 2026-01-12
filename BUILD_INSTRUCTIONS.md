# System Monitor - Visual Studio ile Derleme

## ⚠️ ÖNEMLİ: Visual Studio Kullanın!

Bu proje **Windows SDK** gerektirir. VS Code'da clang IntelliSense Windows.h'yi bulamaz, bu NORMAL.

## Derleme Adımları

### Yöntem 1: Visual Studio ile Doğrudan Aç (ÖNERİLEN)

1. **Visual Studio 2019 veya 2022** açın
2. **File → Open → CMake...** seçin
3. `d:/C_C++ Projects/System Monitor/CMakeLists.txt` dosyasını seçin
4. Visual Studio otomatik olarak projeyi yapılandıracak
5. **Build → Build All** veya `F7` tuşuna basın
6. **Debug → Start** veya `F5` ile çalıştırın

### Yöntem 2: CMake ile Komut Satırından

```powershell
# Proje dizinine gidin
cd "d:/C_C++ Projects/System Monitor"

# Visual Studio 2022 için build
cmake -B build -G "Visual Studio 17 2022"

# Visual Studio 2019 için build  
cmake -B build -G "Visual Studio 16 2019"

# Derleyin
cmake --build build --config Release

# Çalıştırın
./build/Release/SystemMonitor.exe
```

## Gereksinimler

✅ **Visual Studio 2019 veya 2022** (Community Edition yeterli)
- Kurulumda **"Desktop development with C++"** workload'ı seçin
- Bu Windows SDK'yı otomatik kurar

## Hata Giderme

### "Windows.h not found" hatası
- **Çözüm**: Visual Studio'da **MSVC compiler** kullanın
- VS Code clang IntelliSense bu hatayı gösterir ama MSVC ile derleme çalışır

### CMake bulunamıyor
- Visual Studio varsa, doğrudan CMakeLists.txt'yi açın (Yöntem 1)
- Ya da CMake'i indirin: https://cmake.org/download/

### GPU izleme çalışmıyor
- Normal: GPU PDH counter'ları bazı sistemlerde mevcut değildir
- Program yine de çalışır, GPU widget "not available" gösterir

## Başarılı Derleme Sonrası

Program bir pencere açacak ve 4 widget gösterecek:
- 🟢 CPU Usage
- 🔵 Memory Usage  
- 🟣 Disk Usage (C:)
- 🟡 GPU Usage

Pencere dark theme ile modern görünümlü olacak! 🎨
