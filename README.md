# System Monitor

Gerçek zamanlı sistem kaynaklarını izleyen modern bir masaüstü uygulaması.

## Özellikler

- CPU kullanım yüzdesi
- RAM kullanımı ve toplam bellek bilgisi
- Disk alanı kullanımı (C: sürücüsü)
- GPU kullanımı ve VRAM takibi
- Pil durumu ve güç yönetimi
- Modern grafiksel arayüz

## Gereksinimler

### Yazılım
- Windows 10 veya üstü
- CMake 3.15 veya üstü
- Visual Studio 2019/2022 veya MinGW derleyici
- OpenGL desteği (genellikle sistem ile gelir)

### Donanım
- Herhangi bir Windows bilgisayar
- GPU izleme için uyumlu ekran kartı sürücüleri (opsiyonel)

## Kurulum ve Derleme

### Yöntem 1: Build Script ile (Önerilen)

```powershell
# Proje dizinine gidin
cd "d:/C_C++ Projects/System Monitor"

# Build script'i çalıştırın
./build.ps1
```

### Yöntem 2: Manuel CMake

```powershell
# Proje dizinine gidin
cd "d:/C_C++ Projects/System Monitor"

# Build dosyalarını oluşturun
cmake -B build -G "Visual Studio 17 2022"

# Projeyi derleyin
cmake --build build --config Release

# Programı çalıştırın
./build/Release/SystemMonitor.exe
```

### Yöntem 3: Visual Studio ile

1. Visual Studio 2019 veya 2022'yi açın
2. File > Open > CMake seçin
3. CMakeLists.txt dosyasını seçin
4. Build > Build All (F7)
5. Debug > Start (F5)

## Kullanım

Program başlatıldığında beş farklı bilgi paneli gösterir:

- **CPU**: Anlık işlemci kullanım yüzdesi
- **Memory**: Kullanılan ve toplam RAM bilgisi
- **Disk**: C sürücüsünün doluluk durumu
- **GPU**: Ekran kartı kullanımı (uyumluysa)
- **Power**: Pil durumu, AC/Battery modu, kalan süre

Bilgiler saniyede bir otomatik güncellenir. Pencereyi kapatarak programdan çıkabilirsiniz.

## Teknik Detaylar

### Kullanılan Teknolojiler
- C++17 standardı
- ImGui (Dear ImGui) - GUI framework
- GLFW - Pencere yönetimi
- OpenGL 3.0 - Grafik rendering
- Windows PDH API - CPU ve GPU izleme
- Windows API - Bellek, disk ve güç yönetimi

### Proje Yapısı
```
System Monitor/
├── src/                    # Kaynak kod dosyaları
│   ├── main.cpp           # Ana program
│   ├── cpu_monitor.cpp    # CPU izleme
│   ├── memory_monitor.cpp # Bellek izleme
│   ├── disk_monitor.cpp   # Disk izleme
│   ├── gpu_monitor.cpp    # GPU izleme
│   ├── power_monitor.cpp  # Güç izleme
│   └── gui.cpp            # Grafiksel arayüz
├── include/               # Header dosyaları
├── external/              # Harici kütüphaneler
│   ├── imgui/            # ImGui kütüphanesi
│   └── glfw/             # GLFW kütüphanesi
└── CMakeLists.txt        # CMake yapılandırması
```

## Sorun Giderme

### "Windows.h bulunamadı" hatası
Visual Studio'da MSVC derleyicisi kullanarak derlemeyi deneyin. VS Code'daki IntelliSense hataları görmezden gelebilirsiniz.

### GPU izleme çalışmıyor
GPU performans sayaçları bazı sistemlerde mevcut olmayabilir. Program GPU widget'ında "not available" mesajı gösterecektir.

### CMake bulunamıyor
Visual Studio kuruluysa, doğrudan CMakeLists.txt dosyasını Visual Studio ile açabilirsiniz.

## Lisans

MIT License

## Geliştirici Notları

Proje öğrenme amaçlı geliştirilmiştir. Windows sistem programlama, GUI geliştirme ve gerçek zamanlı veri izleme konularında pratik yapmak için uygundur.
