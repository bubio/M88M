param (
    [string]$Action = ""
)

# M88M Windows Build Script (PowerShell)
# Requirement: Visual Studio 2022 and CMake

$BuildDir = "build"
$Config = "RelWithDebInfo"

if ($Action -eq "clean") {
    Write-Host "Cleaning build directory..."
    if (Test-Path $BuildDir) {
        Remove-Item -Recurse -Force $BuildDir
    }
    exit 0
}

if (-not (Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
}

Write-Host "Configuring project..."
cmake -S . -B $BuildDir -G "Visual Studio 17 2022" -A x64
if ($LASTEXITCODE -ne 0) {
    Write-Error "Configuration failed."
    exit $LASTEXITCODE
}

Write-Host "Building project..."
cmake --build $BuildDir --config $Config --target m88_raylib
if ($LASTEXITCODE -ne 0) {
    Write-Error "Build failed."
    exit $LASTEXITCODE
}

Write-Host "`nBuild successful!"
Write-Host "Executable is located at: $BuildDir\$Config\m88m.exe`n"

if ($Action -eq "run") {
    Write-Host "Starting m88m.exe..."
    Push-Location "$BuildDir\$Config"
    try {
        .\m88m.exe
    } finally {
        Pop-Location
    }
}
