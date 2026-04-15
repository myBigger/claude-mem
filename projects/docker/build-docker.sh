#!/bin/bash
# PrisonSIS Docker 编译脚本
# 支持 x86_64 和 ARM64 架构
# 支持在 ARM64 主机上交叉编译 x86_64 程序

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 获取脚本目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${PROJECT_DIR}/build-docker"

# 显示帮助
show_help() {
    echo "PrisonSIS Docker 编译脚本"
    echo ""
    echo "用法: $0 [选项]"
    echo ""
    echo "选项:"
    echo "  -a, --arch <架构>    目标架构 (x86_64, arm64) [默认: 自动检测]"
    echo "  -c, --cross         交叉编译模式 (ARM64 主机编译 x86_64)"
    echo "  -r, --release       Release 构建 [默认: Debug]"
    echo "  -o, --output <目录>  输出目录 [默认: build-docker]"
    echo "  -c, --clean         清理后重新构建"
    echo "  -s, --shell         进入容器 shell"
    echo "  -h, --help          显示帮助"
    echo ""
    echo "示例:"
    echo "  $0                       # 自动检测架构并构建"
    echo "  $0 --arch x86_64         # 编译 x86_64 版本"
    echo "  $0 --cross               # ARM64 主机上交叉编译 x86_64"
    echo "  $0 --cross --release     # ARM64 主机编译 Release 版本"
    echo "  $0 --clean --shell        # 清理并进入 shell"
}

# 检测当前架构
detect_arch() {
    local arch=$(uname -m)
    case $arch in
        x86_64)
            echo "x86_64"
            ;;
        aarch64|arm64)
            echo "arm64"
            ;;
        *)
            echo "unknown"
            ;;
    esac
}

# 检查 Docker
check_docker() {
    if ! command -v docker &> /dev/null; then
        echo -e "${RED}错误: Docker 未安装${NC}"
        echo "请安装 Docker: https://docs.docker.com/get-docker/"
        exit 1
    fi
    if ! docker info &> /dev/null; then
        echo -e "${RED}错误: Docker 服务未运行${NC}"
        echo "请启动 Docker Desktop 或 docker 服务"
        exit 1
    fi
}

# 解析参数
TARGET_ARCH=""
BUILD_TYPE="Debug"
CLEAN=""
SHELL_MODE=""
CROSS_COMPILE=""
OUTPUT_DIR="build-docker"

while [[ $# -gt 0 ]]; do
    case $1 in
        -a|--arch)
            TARGET_ARCH="$2"
            shift 2
            ;;
        -c|--cross)
            CROSS_COMPILE="yes"
            shift
            ;;
        -r|--release)
            BUILD_TYPE="Release"
            shift
            ;;
        -o|--output)
            OUTPUT_DIR="$2"
            shift 2
            ;;
        --clean)
            CLEAN="yes"
            shift
            ;;
        -s|--shell)
            SHELL_MODE="yes"
            shift
            ;;
        -h|--help)
            show_help
            exit 0
            ;;
        *)
            echo -e "${RED}未知选项: $1${NC}"
            show_help
            exit 1
            ;;
    esac
done

BUILD_DIR="${PROJECT_DIR}/${OUTPUT_DIR}"

# 如果是交叉编译模式
if [ "$CROSS_COMPILE" = "yes" ]; then
    CURRENT_ARCH=$(detect_arch)

    if [ "$CURRENT_ARCH" = "arm64" ]; then
        echo -e "${BLUE}========================================${NC}"
        echo -e "${BLUE}PrisonSIS 交叉编译模式${NC}"
        echo -e "${BLUE}========================================${NC}"
        echo "主机架构: ARM64 (aarch64)"
        echo "目标架构: x86_64"
        echo "构建类型: $BUILD_TYPE"
        echo -e "${BLUE}========================================${NC}"

        check_docker

        DOCKERFILE="${SCRIPT_DIR}/Dockerfile.arm64-cross"
        IMAGE_NAME="prisonsis-build:arm64-cross-x86"
        DOCKER_EXTRA_ARGS="--privileged"
        TARGET_ARCH="x86_64"
    else
        echo -e "${RED}错误: 交叉编译模式仅支持在 ARM64 主机上运行${NC}"
        echo "当前主机架构: $CURRENT_ARCH"
        exit 1
    fi
else
    # 如果未指定架构，自动检测
    if [ -z "$TARGET_ARCH" ]; then
        CURRENT_ARCH=$(detect_arch)
        echo -e "${YELLOW}检测到当前架构: $CURRENT_ARCH${NC}"

        # 默认编译本机架构
        if [ "$CURRENT_ARCH" = "x86_64" ]; then
            echo -e "${GREEN}将构建 x86_64 版本${NC}"
            TARGET_ARCH="x86_64"
        else
            echo -e "${GREEN}将构建 ARM64 版本${NC}"
            TARGET_ARCH="arm64"
        fi
    fi

    # 设置 Dockerfile
    if [ "$TARGET_ARCH" = "x86_64" ]; then
        DOCKERFILE="${SCRIPT_DIR}/Dockerfile.x86_64"
    elif [ "$TARGET_ARCH" = "arm64" ]; then
        DOCKERFILE="${SCRIPT_DIR}/Dockerfile.arm64-cross"
    else
        echo -e "${RED}不支持的架构: $TARGET_ARCH${NC}"
        exit 1
    fi

    IMAGE_NAME="prisonsis-build:${TARGET_ARCH}"

    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}PrisonSIS Docker 编译${NC}"
    echo -e "${GREEN}========================================${NC}"
    echo "目标架构: $TARGET_ARCH"
    echo "构建类型: $BUILD_TYPE"
    echo "Dockerfile: $DOCKERFILE"
    echo -e "${GREEN}========================================${NC}"

    check_docker
fi

CONTAINER_NAME="prisonsis-builder-${TARGET_ARCH}"

# 清理旧构建
if [ "$CLEAN" = "yes" ]; then
    echo -e "${YELLOW}清理构建目录...${NC}"
    rm -rf "$BUILD_DIR"
fi

# 创建构建目录
mkdir -p "$BUILD_DIR"

# 如果需要进入 shell
if [ "$SHELL_MODE" = "yes" ]; then
    echo -e "${YELLOW}启动容器 shell...${NC}"
    docker run --rm -it \
        -v "${PROJECT_DIR}:/workspace/PrisonSIS" \
        -w /workspace/PrisonSIS \
        --name "$CONTAINER_NAME" \
        -e BUILD_TYPE="$BUILD_TYPE" \
        ${DOCKER_EXTRA_ARGS:-} \
        "$IMAGE_NAME" \
        /bin/bash
    exit 0
fi

# 构建 Docker 镜像
echo -e "${YELLOW}构建 Docker 镜像...${NC}"
docker build -t "$IMAGE_NAME" -f "$DOCKERFILE" "$SCRIPT_DIR"

# 运行编译
echo -e "${YELLOW}在容器中编译项目...${NC}"

# binfmt_misc 注册脚本（让 ARM64 主机通过 QEMU 运行 x86_64 工具）
REGISTER_BINFMT='
    if [ -w /proc/sys/fs/binfmt_misc/register ]; then
        echo ":qemu-x86_64:M::\\x7fELF\\x02:\\x00\\x10\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x02\\x00\\x3e\\x00:^\\x7fELF\\x02\\x00\\x3e\\x00:\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00:\\xff\\xff\\xff\\xff\\xff\\xff\\xff\\x00\\xff\\xff\\xff\\xff\\xff\\xff\\xff:@ /usr/bin/qemu-x86_64-static:" > /proc/sys/fs/binfmt_misc/register 2>/dev/null && echo "QEMU x86_64 binfmt registered" || echo "binfmt register skipped (may already exist)"
    else
        echo "binfmt_misc not writable, skipping (host may already handle it)"
    fi
'

docker run --rm \
    -v "${PROJECT_DIR}:/workspace/PrisonSIS" \
    -w /workspace/PrisonSIS \
    -e BUILD_TYPE="$BUILD_TYPE" \
    --name "$CONTAINER_NAME" \
    ${DOCKER_EXTRA_ARGS:-} \
    "$IMAGE_NAME" \
    sh -c "
        set -e
        ${REGISTER_BINFMT}
        mkdir -p build-docker
        cd build-docker

        echo '运行 CMake 配置 (交叉编译: ARM64 主机 → x86_64 目标)...'
        cmake .. \
            -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
            -DCMAKE_TOOLCHAIN_FILE=/workspace/PrisonSIS/docker/toolchain-x86_64.cmake \
            -GNinja

        echo '开始编译...'
        cmake --build . --parallel

        echo ''
        echo '========================================'
        echo -e '${GREEN}编译完成！${NC}'
        echo '========================================'
        echo '可执行文件位置:'
        find . -name 'PrisonRecordTool' -type f -executable
    "

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}Docker 编译完成！${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""
echo "编译产物位于: ${BUILD_DIR}/bin/PrisonRecordTool"
echo ""
echo "如需进入容器调试，运行:"
echo "  $0 --shell --arch $TARGET_ARCH"
