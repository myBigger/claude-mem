# PrisonSIS Docker 交叉编译指南

本目录包含用于跨平台编译 PrisonSIS 的 Docker 配置。

## 🎯 支持的编译场景

| 当前架构 | 目标架构 | 用途 |
|---------|---------|------|
| x86_64 | x86_64 | 本地编译 |
| x86_64 | ARM64 | Mac M系列 / 树莓派 |
| ARM64 | x86_64 | 嵌入式设备交叉编译 |
| ARM64 | ARM64 | 本地编译 |

## 📋 前置要求

- Docker 20.10+
- docker-compose 2.0+（可选）

## 🚀 快速开始

### 方法一：使用构建脚本（推荐）

```bash
# 进入 docker 目录
cd docker

# 给脚本添加执行权限
chmod +x build-docker.sh

# 自动检测架构并编译
./build-docker.sh

# 指定目标架构
./build-docker.sh --arch x86_64    # 编译 x86_64 版本
./build-docker.sh --arch arm64      # 编译 ARM64 版本

# Release 版本
./build-docker.sh --release

# 清理后重新编译
./build-docker.sh --clean

# 进入容器 shell 调试
./build-docker.sh --shell --arch x86_64
```

### 方法二：使用 Docker Compose

```bash
cd docker

# 编译 x86_64 版本
docker-compose --profile build run build-x86_64

# 编译 ARM64 版本
docker-compose --profile build run build-arm64

# 进入 shell 调试
docker-compose --profile shell run shell-x86_64
```

### 方法三：手动 Docker 命令

```bash
cd docker

# 构建镜像
docker build -t prisonsis-build:x86_64 -f Dockerfile.x86_64 .

# 运行编译
docker run --rm \
    -v $(pwd)/..:/workspace/PrisonSIS \
    -w /workspace/PrisonSIS \
    prisonsis-build:x86_64 \
    sh -c "mkdir -p build-docker && cd build-docker && cmake .. && cmake --build . --parallel"
```

## 📁 生成的二进制文件

编译完成后，二进制文件位于：

```
PrisonSIS/
├── build-docker/
│   └── bin/
│       └── PrisonRecordTool    # 可执行文件
```

## 🔧 CMake 工具链文件

如需更精细的交叉编译控制，可以使用 CMake 工具链文件：

### x86_64 工具链文件 (arm64 -> x86_64)

创建 `docker/toolchain-x86_64.cmake`:

```cmake
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# 交叉编译器路径
set(CMAKE_C_COMPILER x86_64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER x86_64-linux-gnu-g++)

# Qt 路径
set(Qt6_DIR /usr/lib/x86_64-linux-gnu/cmake/Qt6)

# 搜索路径
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-linux-gnu)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
```

使用：
```bash
cd build-docker
cmake .. -DCMAKE_TOOLCHAIN_FILE=../docker/toolchain-x86_64.cmake
cmake --build .
```

## 🐛 常见问题

### Q: Docker 构建失败

```bash
# 清理 Docker 缓存
docker system prune -a
docker builder prune
```

### Q: 找不到 Qt6 模块

确保安装了完整的 Qt6 开发包：
```bash
apt-get install qt6-base-dev qt6-base-dev-tools
```

### Q: 编译内存不足

增加 Docker 内存限制（Docker Desktop 设置 > Resources > Memory）

## 📦 Windows/macOS 安装 Docker

- **Windows**: https://docs.docker.com/desktop/install/windows-install/
- **macOS**: https://docs.docker.com/desktop/install/mac-install/

安装 WSL2（Windows）后性能更好。

## 📝 自定义 Dockerfile

如需添加其他依赖，修改对应的 Dockerfile：

```dockerfile
RUN apt-get update && apt-get install -y \
    your-package-here \
    && apt-get clean && rm -rf /var/lib/apt/lists/*
```
