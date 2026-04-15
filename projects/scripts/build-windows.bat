@echo off
REM ============================================================
REM PrisonRecordTool Windows Build Script
REM Windows AMD64 Platform Build Script
REM ============================================================
REM
REM 依赖要求:
REM   - Visual Studio 2022 或 Build Tools for Visual Studio 2022
REM   - CMake 3.20+
REM   - Qt 6.x for MSVC (https://www.qt.io/download-qt-installer)
REM   - OpenSSL 3.x (https://slproweb.com/products/Win32OpenSSL.html)
REM
REM 使用方法:
REM   1. 安装依赖
REM   2. 设置环境变量 (见下方配置部分)
REM   3. 双击运行本脚本，或在 VS Developer Command Prompt 中运行
REM
REM ============================================================

setlocal EnableDelayedExpansion

REM ============================================================
REM 配置区域 - 请根据您的安装路径修改
REM ============================================================

REM Qt 安装路径 (修改为您的 Qt 安装路径)
set QT_PATH=C:\Qt\6.7.0\msvc2022_64

REM OpenSSL 安装路径
set OPENSSL_PATH=C:\Program Files\OpenSSL-Win64

REM Visual Studio 路径 (如果使用 Build Tools)
set VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\BuildTools

REM ============================================================
REM 自动检测
REM ============================================================

REM 检测 Qt
if not exist "%QT_PATH%" (
    echo [错误] Qt 路径不存在: %QT_PATH%
    echo 请修改脚本中的 QT_PATH 变量，或安装 Qt 6.x
    echo.
    echo 提示: 您可以从 https://www.qt.io/download-qt-installer 下载 Qt
    pause
    exit /b 1
)

REM 检测 OpenSSL
if not exist "%OPENSSL_PATH%" (
    echo [警告] OpenSSL 路径不存在: %OPENSSL_PATH%
    echo 请安装 OpenSSL 3.x (https://slproweb.com/products/Win32OpenSSL.html)
    echo 或修改脚本中的 OPENSSL_PATH 变量
)

REM ============================================================
REM 设置环境变量
REM ============================================================

echo [配置] 设置构建环境...

REM Qt 环境
set PATH=%QT_PATH%\bin;%PATH%
set PATH=%QT_PATH%\lib;%PATH%
set Qt6_DIR=%QT_PATH%\lib\cmake\Qt6

REM OpenSSL 环境
if exist "%OPENSSL_PATH%" (
    set PATH=%OPENSSL_PATH%\bin;%PATH%
    set OPENSSL_ROOT=%OPENSSL_PATH%
    set OPENSSL_INCLUDE_DIR=%OPENSSL_PATH%\include
    set OPENSSL_LIBRARY_DIR=%OPENSSL_PATH%\lib
)

REM ============================================================
REM 构建配置
REM ============================================================

set BUILD_DIR=%~dp0build-windows
set INSTALL_DIR=%~dp0install-windows

echo.
echo ============================================================
echo   PrisonRecordTool Windows 构建脚本
echo ============================================================
echo.
echo 配置:
echo   Qt 路径:      %QT_PATH%
echo   OpenSSL 路径: %OPENSSL_PATH%
echo   构建目录:     %BUILD_DIR%
echo   安装目录:     %INSTALL_DIR%
echo.
echo ============================================================
echo.

REM 清理旧的构建目录
if exist "%BUILD_DIR%" (
    echo [清理] 删除旧的构建目录...
    rmdir /s /q "%BUILD_DIR%"
)

REM 创建构建目录
echo [创建] 创建构建目录...
mkdir "%BUILD_DIR%"

REM ============================================================
REM CMake 配置
REM ============================================================

echo [CMake] 配置项目...

cd /d "%BUILD_DIR%"

cmake -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_INSTALL_PREFIX="%INSTALL_DIR%" ^
    -DQt6_DIR="%Qt6_DIR%" ^
    "%~dp0"

if errorlevel 1 (
    echo.
    echo [错误] CMake 配置失败!
    echo.
    echo 常见问题:
    echo   1. 确认已安装 Visual Studio 2022 Build Tools
    echo   2. 确认 Qt 6.x 已正确安装 (msvc2022_64 版本)
    echo   3. 确认 OpenSSL 已安装
    echo.
    pause
    exit /b 1
)

REM ============================================================
REM 编译
REM ============================================================

echo.
echo [编译] 开始编译项目...
echo.

cmake --build . --config Release --parallel

if errorlevel 1 (
    echo.
    echo [错误] 编译失败!
    pause
    exit /b 1
)

REM ============================================================
REM 安装
REM ============================================================

echo.
echo [安装] 安装到 %INSTALL_DIR%...

cmake --install . --config Release

REM ============================================================
REM 复制依赖 DLL
REM ============================================================

echo.
echo [依赖] 复制运行时依赖...

set BIN_DIR=%INSTALL_DIR%\bin
if not exist "%BIN_DIR%" mkdir "%BIN_DIR%"

REM 复制 Qt 运行时 DLL
echo   - 复制 Qt 运行时...
xcopy /y /q "%QT_PATH%\bin\Qt6Core.dll" "%BIN_DIR%\" 2>nul
xcopy /y /q "%QT_PATH%\bin\Qt6Gui.dll" "%BIN_DIR%\" 2>nul
xcopy /y /q "%QT_PATH%\bin\Qt6Widgets.dll" "%BIN_DIR%\" 2>nul
xcopy /y /q "%QT_PATH%\bin\Qt6Sql.dll" "%BIN_DIR%\" 2>nul
xcopy /y /q "%QT_PATH%\bin\Qt6Xml.dll" "%BIN_DIR%\" 2>nul
xcopy /y /q "%QT_PATH%\bin\Qt6PrintSupport.dll" "%BIN_DIR%\" 2>nul
xcopy /y /q "%QT_PATH%\bin\Qt6Network.dll" "%BIN_DIR%\" 2>nul

REM 复制 OpenSSL DLL
if exist "%OPENSSL_PATH%\bin\" (
    echo   - 复制 OpenSSL...
    xcopy /y /q "%OPENSSL_PATH%\bin\libssl-3-x64.dll" "%BIN_DIR%\" 2>nul
    xcopy /y /q "%OPENSSL_PATH%\bin\libcrypto-3-x64.dll" "%BIN_DIR%\" 2>nul
)

REM 复制 MSVC 运行时
echo   - 复制 MSVC 运行时...
if exist "C:\Windows\System32\vcruntime140.dll" (
    xcopy /y /q "C:\Windows\System32\vcruntime140.dll" "%BIN_DIR%\" 2>nul
)
if exist "C:\Windows\System32\vcruntime140_1.dll" (
    xcopy /y /q "C:\Windows\System32\vcruntime140_1.dll" "%BIN_DIR%\" 2>nul
)
if exist "C:\Windows\System32\msvcp140.dll" (
    xcopy /y /q "C:\Windows\System32\msvcp140.dll" "%BIN_DIR%\" 2>nul
)

REM ============================================================
REM 复制 Qt 插件
REM ============================================================

echo   - 复制 Qt 插件...

set PLUGINS_DIR=%INSTALL_DIR%\plugins
if not exist "%PLUGINS_DIR%" mkdir "%PLUGINS_DIR%"

REM 复制 platforms 插件 (窗口系统)
if exist "%QT_PATH%\plugins\platforms\" (
    if not exist "%PLUGINS_DIR%\platforms" mkdir "%PLUGINS_DIR%\platforms"
    xcopy /y /q "%QT_PATH%\plugins\platforms\qwindows.dll" "%PLUGINS_DIR%\platforms\" 2>nul
)

REM 复制 styles 插件
if exist "%QT_PATH%\plugins\styles\" (
    if not exist "%PLUGINS_DIR%\styles" mkdir "%PLUGINS_DIR%\styles"
    xcopy /y /q "%QT_PATH%\plugins\styles\*" "%PLUGINS_DIR%\styles\" 2>nul
)

REM ============================================================
完成
REM ============================================================

echo.
echo ============================================================
echo   构建完成!
echo ============================================================
echo.
echo 可执行文件: %INSTALL_DIR%\bin\PrisonRecordTool.exe
echo.
echo 运行前请确保所有依赖 DLL 已在同一目录或系统 PATH 中。
echo.
echo 提示: 可以使用 windeployqt 工具自动部署所有 Qt 依赖:
echo   windeployqt "%INSTALL_DIR%\bin\PrisonRecordTool.exe"
echo.
echo ============================================================

pause
