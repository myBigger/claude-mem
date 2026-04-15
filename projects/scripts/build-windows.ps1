#Requires -Version 5.1
<#
.SYNOPSIS
    PrisonRecordTool Windows Build Script (PowerShell)

.DESCRIPTION
    使用 PowerShell 构建 PrisonRecordTool for Windows AMD64

.PARAMETER QtPath
    Qt 安装路径 (默认: C:\Qt\6.7.0\msvc2022_64)

.PARAMETER OpenSSLPath
    OpenSSL 安装路径 (默认: C:\Program Files\OpenSSL-Win64)

.PARAMETER BuildType
    构建类型: Release 或 Debug (默认: Release)

.PARAMETER Clean
    清理后重新构建

.EXAMPLE
    .\build-windows.ps1
    .\build-windows.ps1 -QtPath "D:\Qt\6.6.0\msvc2022_64" -Clean
    .\build-windows.ps1 -BuildType Debug
#>

param(
    [string]$QtPath = "C:\Qt\6.7.0\msvc2022_64",
    [string]$OpenSSLPath = "C:\Program Files\OpenSSL-Win64",
    [ValidateSet("Release", "Debug")]
    [string]$BuildType = "Release",
    [switch]$Clean
)

$ErrorActionPreference = "Stop"
$ProjectRoot = Split-Path -Parent $PSScriptRoot

# ============================================================
# 打印标题
# ============================================================
Write-Host ""
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "  PrisonRecordTool Windows 构建脚本 (PowerShell)" -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host ""

# ============================================================
# 配置检查
# ============================================================
Write-Host "[1/5] 检查依赖..." -ForegroundColor Yellow

# 检查 Qt
if (-not (Test-Path $QtPath)) {
    Write-Host "  [错误] Qt 路径不存在: $QtPath" -ForegroundColor Red
    Write-Host "  请从 https://www.qt.io/download-qt-installer 下载 Qt 6.x (MSVC 2022 64-bit)" -ForegroundColor Yellow
    Write-Host "  或使用 -QtPath 参数指定正确的路径" -ForegroundColor Yellow
    exit 1
}
Write-Host "  [OK] Qt: $QtPath" -ForegroundColor Green

# 检查 Visual Studio
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vsLocation = & $vswhere -latest -property installationPath -format value 2>$null

if (-not $vsLocation) {
    Write-Host "  [错误] 未找到 Visual Studio" -ForegroundColor Red
    Write-Host "  请安装 Visual Studio 2022 或 Build Tools for Visual Studio 2022" -ForegroundColor Yellow
    exit 1
}
Write-Host "  [OK] Visual Studio: $vsLocation" -ForegroundColor Green

# 检查 CMake
$cmake = Get-Command cmake -ErrorAction SilentlyContinue
if (-not $cmake) {
    Write-Host "  [错误] 未找到 CMake" -ForegroundColor Red
    Write-Host "  请安装 CMake 3.20+ (https://cmake.org/download/)" -ForegroundColor Yellow
    exit 1
}
Write-Host "  [OK] CMake: $($cmake.Version)" -ForegroundColor Green

# 检查 OpenSSL
$opensslOk = Test-Path "$OpenSSLPath\bin\libssl-3-x64.dll"
if (-not $opensslOk) {
    Write-Host "  [警告] OpenSSL DLL 未找到: $OpenSSLPath\bin\libssl-3-x64.dll" -ForegroundColor DarkYellow
    Write-Host "  建议安装 https://slproweb.com/products/Win32OpenSSL.html" -ForegroundColor Yellow
}
else {
    Write-Host "  [OK] OpenSSL: $OpenSSLPath" -ForegroundColor Green
}

# ============================================================
# 环境配置
# ============================================================
Write-Host ""
Write-Host "[2/5] 配置环境..." -ForegroundColor Yellow

# Qt 环境
$env:PATH = "$QtPath\bin;$env:PATH"
$env:Qt6_DIR = "$QtPath\lib\cmake\Qt6"

# OpenSSL 环境
if (Test-Path $OpenSSLPath) {
    $env:PATH = "$OpenSSLPath\bin;$env:PATH"
    $env:OPENSSL_ROOT = $OpenSSLPath
    $env:OPENSSL_INCLUDE_DIR = "$OpenSSLPath\include"
    $env:OPENSSL_LIBRARY_DIR = "$OpenSSLPath\lib"
}

# ============================================================
# 构建目录
# ============================================================
$BuildDir = Join-Path $ProjectRoot "build-windows"
$InstallDir = Join-Path $ProjectRoot "install-windows"

Write-Host ""
Write-Host "[3/5] 构建配置..." -ForegroundColor Yellow
Write-Host "  构建类型: $BuildType"
Write-Host "  构建目录: $BuildDir"
Write-Host "  安装目录: $InstallDir"

# 清理旧构建
if ($Clean -and (Test-Path $BuildDir)) {
    Write-Host "  [清理] 删除旧的构建目录..." -ForegroundColor Cyan
    Remove-Item -Recurse -Force $BuildDir
}

# 创建构建目录
if (-not (Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
}

# ============================================================
# CMake 配置
# ============================================================
Write-Host ""
Write-Host "[4/5] CMake 配置和编译..." -ForegroundColor Yellow

Push-Location $BuildDir

try {
    # CMake 配置
    Write-Host "  运行 CMake 配置..." -ForegroundColor Cyan
    $cmakeArgs = @(
        "-G", "Visual Studio 17 2022",
        "-A", "x64",
        "-DCMAKE_BUILD_TYPE=$BuildType",
        "-DCMAKE_INSTALL_PREFIX=`"$InstallDir`"",
        "-DQt6_DIR=`"$env:Qt6_DIR`""
    )

    if (Test-Path $OpenSSLPath) {
        $cmakeArgs += "-DOPENSSL_ROOT=`"$OpenSSLPath`""
    }

    $cmakeArgs += "`"$ProjectRoot`""

    & cmake @cmakeArgs

    if ($LASTEXITCODE -ne 0) {
        throw "CMake 配置失败"
    }

    # 编译
    Write-Host "  编译项目..." -ForegroundColor Cyan
    & cmake --build . --config $BuildType --parallel

    if ($LASTEXITCODE -ne 0) {
        throw "编译失败"
    }

    # 安装
    Write-Host "  安装到 $InstallDir..." -ForegroundColor Cyan
    & cmake --install . --config $BuildType

    if ($LASTEXITCODE -ne 0) {
        throw "安装失败"
    }
}
finally {
    Pop-Location
}

# ============================================================
# 复制依赖
# ============================================================
Write-Host ""
Write-Host "[5/5] 复制运行时依赖..." -ForegroundColor Yellow

$BinDir = Join-Path $InstallDir "bin"
if (-not (Test-Path $BinDir)) {
    New-Item -ItemType Directory -Path $BinDir | Out-Null
}

# Qt DLL
Write-Host "  复制 Qt 运行时..." -ForegroundColor Cyan
$qtDlls = @(
    "Qt6Core.dll", "Qt6Gui.dll", "Qt6Widgets.dll",
    "Qt6Sql.dll", "Qt6Xml.dll", "Qt6PrintSupport.dll",
    "Qt6Network.dll", "Qt6Pdf.dll"
)

foreach ($dll in $qtDlls) {
    $src = Join-Path $QtPath "bin\$dll"
    if (Test-Path $src) {
        Copy-Item -Force $src $BinDir
    }
}

# OpenSSL DLL
if (Test-Path "$OpenSSLPath\bin") {
    Write-Host "  复制 OpenSSL..." -ForegroundColor Cyan
    Copy-Item -Force "$OpenSSLPath\bin\libssl-3-x64.dll" $BinDir -ErrorAction SilentlyContinue
    Copy-Item -Force "$OpenSSLPath\bin\libcrypto-3-x64.dll" $BinDir -ErrorAction SilentlyContinue
}

# MSVC 运行时
Write-Host "  复制 MSVC 运行时..." -ForegroundColor Cyan
$msvcDlls = @("vcruntime140.dll", "vcruntime140_1.dll", "msvcp140.dll")
foreach ($dll in $msvcDlls) {
    $src = "C:\Windows\System32\$dll"
    if (Test-Path $src) {
        Copy-Item -Force $src $BinDir -ErrorAction SilentlyContinue
    }
}

# Qt 插件
Write-Host "  复制 Qt 插件..." -ForegroundColor Cyan
$PluginsDir = Join-Path $InstallDir "plugins"
$QtPluginsSrc = Join-Path $QtPath "plugins"

# Platforms
$platformsSrc = Join-Path $QtPluginsSrc "platforms\qwindows.dll"
if (Test-Path $platformsSrc) {
    $platformsDest = Join-Path $PluginsDir "platforms"
    if (-not (Test-Path $platformsDest)) {
        New-Item -ItemType Directory -Path $platformsDest | Out-Null
    }
    Copy-Item -Force $platformsSrc $platformsDest
}

# ============================================================
# 完成
# ============================================================
Write-Host ""
Write-Host "============================================================" -ForegroundColor Green
Write-Host "  构建完成!" -ForegroundColor Green
Write-Host "============================================================" -ForegroundColor Green
Write-Host ""
Write-Host "可执行文件: $BinDir\PrisonRecordTool.exe" -ForegroundColor Cyan
Write-Host ""
Write-Host "提示: 可以使用 windeployqt 补充 Qt 依赖:" -ForegroundColor Yellow
Write-Host "  windeployqt `"$BinDir\PrisonRecordTool.exe`"" -ForegroundColor Gray
Write-Host ""

# 自动运行 windeployqt
Write-Host "运行 windeployqt 补充 Qt 依赖..." -ForegroundColor Cyan
& "$QtPath\bin\windeployqt.exe" --no-translations "$BinDir\PrisonRecordTool.exe" 2>$null

Write-Host ""
Write-Host "完成! 可以在 $BinDir 找到 PrisonRecordTool.exe" -ForegroundColor Green
Write-Host ""
