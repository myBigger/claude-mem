# PrisonSIS Windows 构建指南

本文档说明如何在 Windows AMD64 平台上构建 PrisonRecordTool。

## 系统要求

- Windows 10/11 64-bit
- 至少 8GB RAM
- 20GB 可用磁盘空间

## 依赖安装

### 1. Visual Studio 2022

下载并安装 [Visual Studio 2022](https://visualstudio.microsoft.com/downloads/)。

选择以下组件：
- **使用 C++ 的桌面开发** (Desktop development with C++)
- Windows 11 SDK
- MSVC v143 - VS 2022 C++ x64/x86 生成工具

### 2. CMake

下载并安装 [CMake 3.20+](https://cmake.org/download/)。

建议选择 "Add CMake to system PATH for all users"。

### 3. Qt 6.x

1. 下载 [Qt Online Installer](https://www.qt.io/download-qt-installer)
2. 安装时选择:
   - **Qt 6.7.x** (或更高版本)
   - **MSVC 2022 64-bit** (Qt 6.x)
   - **Qt 6.x.x > Additional Libraries > Qt Network ...**

3. 记住安装路径，默认是: `C:\Qt\6.7.0\msvc2022_64`

### 4. OpenSSL

下载并安装 [OpenSSL 3.x for Windows](https://slproweb.com/products/Win32OpenSSL.html)。

选择 **Win64 OpenSSL 3.x** (非 Light 版本)。

安装路径建议: `C:\Program Files\OpenSSL-Win64`

## 构建步骤

### 方法一: 使用 PowerShell 脚本 (推荐)

```powershell
# 1. 打开 PowerShell (以管理员身份)

# 2. 进入项目目录
cd C:\path\to\PrisonSIS

# 3. 运行构建脚本
.\scripts\build-windows.ps1
```

如果需要指定自定义路径:
```powershell
.\scripts\build-windows.ps1 -QtPath "D:\Qt\6.6.0\msvc2022_64" -OpenSSLPath "D:\OpenSSL-Win64"
```

### 方法二: 手动构建

```batch
REM 1. 打开 "x64 Native Tools Command Prompt for VS 2022"

REM 2. 进入项目目录
cd C:\path\to\PrisonSIS

REM 3. 创建并进入构建目录
mkdir build-windows
cd build-windows

REM 4. CMake 配置
cmake -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_INSTALL_PREFIX=..\install-windows ^
    -DQt6_DIR=C:\Qt\6.7.0\msvc2022_64\lib\cmake\Qt6 ^
    ..

REM 5. 编译
cmake --build . --config Release --parallel

REM 6. 安装
cmake --install . --config Release
```

### 方法三: 使用 windeployqt 部署

```powershell
# 编译完成后，使用 windeployqt 补充所有 Qt 依赖

# 进入安装目录
cd C:\path\to\PrisonSIS\install-windows\bin

# 运行 windeployqt
C:\Qt\6.7.0\msvc2022_64\bin\windeployqt.exe PrisonRecordTool.exe
```

## 运行程序

构建完成后，可执行文件位于:
```
install-windows\bin\PrisonRecordTool.exe
```

首次运行时会:
1. 创建数据库 (`%APPDATA%\PrisonSystem\prison_record.db`)
2. 要求设置 PIN 码 (用于加密主密钥)
3. 创建用户账户

## 常见问题

### Q: CMake 找不到 Qt6

确保设置了 `Qt6_DIR` 环境变量或 CMake 参数:
```cmake
-DQt6_DIR=C:\Qt\6.7.0\msvc2022_64\lib\cmake\Qt6
```

### Q: 链接错误 (linker errors)

1. 确认安装了所有必需的 Visual Studio 组件
2. 确保 Qt 和 OpenSSL 库与构建目标 (x64) 匹配

### Q: 程序启动失败 (DLL not found)

运行 `windeployqt` 来自动复制所有必需的 DLL:
```powershell
C:\Qt\6.7.0\msvc2022_64\bin\windeployqt.exe install-windows\bin\PrisonRecordTool.exe
```

### Q: OpenSSL DLL 未找到

安装 OpenSSL 后，确保其 bin 目录在系统 PATH 中:
```powershell
$env:PATH = "C:\Program Files\OpenSSL-Win64\bin;$env:PATH"
```

## 文件结构 (Windows)

```
%APPDATA%\PrisonSystem\
├── prison_record.db       # SQLite 数据库
├── config\
│   └── master.key.enc     # 加密主密钥
├── keys\
│   └── {userId}\
│       ├── public_key.pem # RSA 公钥
│       └── private_key.enc # PIN 加密的私钥
└── p2p\
    ├── scans\            # 加密扫描件
    └── indices\          # 扫描件索引
```

## 卸载

删除以下目录即可完全卸载:
- `%APPDATA%\PrisonSystem\` (数据目录)
- `C:\Program Files\PrisonRecordTool\` (安装目录，可选)

## 技术支持

如遇到问题，请检查:
1. 所有依赖是否正确安装
2. 环境变量是否配置正确
3. 查看构建日志中的错误信息
