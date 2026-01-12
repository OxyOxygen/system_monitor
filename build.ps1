# System Monitor - Build and Run Script

Write-Host "🔨 System Monitor Build Script" -ForegroundColor Cyan
Write-Host ""

# Check if build directory exists
if (-not (Test-Path "build")) {
    Write-Host "📁 Creating build directory..." -ForegroundColor Yellow
    cmake -B build -G "Visual Studio 17 2022"
    if ($LASTEXITCODE -ne 0) {
        Write-Host "❌ CMake configuration failed!" -ForegroundColor Red
        exit 1
    }
}

# Build the project
Write-Host "🔨 Building project..." -ForegroundColor Yellow
cmake --build build --config Release

if ($LASTEXITCODE -ne 0) {
    Write-Host "❌ Build failed!" -ForegroundColor Red
    exit 1
}

Write-Host "✅ Build successful!" -ForegroundColor Green
Write-Host ""

# Ask if user wants to run
$run = Read-Host "🚀 Run SystemMonitor? (Y/n)"
if ($run -ne "n" -and $run -ne "N") {
    Write-Host "▶️ Starting System Monitor..." -ForegroundColor Cyan
    & "./build/Release/SystemMonitor.exe"
} else {
    Write-Host "👋 Build complete. Run with: ./build/Release/SystemMonitor.exe" -ForegroundColor Green
}
