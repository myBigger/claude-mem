# CMake Toolchain File for Cross-Compilation
# 用于在 ARM64 主机上交叉编译 x86_64 程序
#
# 使用方法:
#   cmake .. -DCMAKE_TOOLCHAIN_FILE=../docker/toolchain-x86_64.cmake

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# 交叉编译器前缀 (Debian multiarch)
set(CROSS_PREFIX x86_64-linux-gnu-)

# 交叉编译器
set(CMAKE_C_COMPILER /usr/bin/${CROSS_PREFIX}gcc)
set(CMAKE_CXX_COMPILER /usr/bin/${CROSS_PREFIX}g++)

# 归档工具
set(CMAKE_AR /usr/bin/${CROSS_PREFIX}ar CACHE FILEPATH "Archiver")
set(CMAKE_RANLIB /usr/bin/${CROSS_PREFIX}ranlib CACHE FILEPATH "Ranlib")

# Debian multiarch 根路径 (x86_64)
set(X86_ROOT /usr/x86_64-linux-gnu)
set(X86_LIB  /usr/lib/x86_64-linux-gnu)

# Qt6 配置
set(Qt6_DIR ${X86_LIB}/cmake/Qt6)

# OpenSSL 配置
set(OPENSSL_INCLUDE_DIR /usr/include/x86_64-linux-gnu)
set(OPENSSL_LIBRARIES  ${X86_LIB})

# Vulkan/GL 搜索路径
set(VULKAN_LIBRARY   ${X86_LIB}/libvulkan.so   CACHE PATH "")
set(OPENGL_LIBRARY   ${X86_LIB}/libGL.so.1     CACHE PATH "")
set(OPENGL_glx_LIBRARY ${X86_LIB}/libGLX.so.0   CACHE PATH "")
set(OPENGL_egl_LIBRARY ${X86_LIB}/libEGL.so.1   CACHE PATH "")
set(OPENGL_GLU_LIBRARY ${X86_LIB}/libGLU.so.1    CACHE PATH "")
set(OPENGL_INCLUDE_DIR /usr/include              CACHE PATH "")

# 搜索路径 — 包含 multiarch 标准路径
set(CMAKE_FIND_ROOT_PATH ${X86_ROOT} ${X86_LIB})

# 不在搜索路径中搜索程序
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# Debian multiarch 混合模式：优先目标系统库，允许回退到系统路径
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)

# 追加 x86_64 多架构路径（跨架构混合构建）
list(APPEND CMAKE_LIBRARY_PATH
    /usr/lib/x86_64-linux-gnu
    /usr/lib
)
list(APPEND CMAKE_INCLUDE_PATH
    /usr/include/x86_64-linux-gnu
)

# pkg-config 路径
set(PKG_CONFIG_PATH ${X86_LIB}/pkgconfig:${X86_ROOT}/lib/pkgconfig)
set(ENV{PKG_CONFIG_PATH} ${PKG_CONFIG_PATH})

# Qt 插件路径
set(QT_QPA_PLATFORM_PLUGINS ${X86_LIB}/qt6/plugins/platforms)

# Qt6 工具链
# moc/uic: 使用 ARM64 原生版本（直接运行）
# 编译: 使用 x86_64 交叉编译器
set(Qt6_moc_EXECUTABLE   /usr/lib/qt6/libexec/moc   CACHE FILEPATH "Qt6 moc (ARM64 native)")
set(Qt6_uic_EXECUTABLE   /usr/lib/qt6/libexec/uic   CACHE FILEPATH "Qt6 uic (ARM64 native)")
set(Qt6_rcc_EXECUTABLE   /usr/lib/qt6/libexec/rcc   CACHE FILEPATH "Qt6 rcc (ARM64 native)")

# CMAKE_PREFIX_PATH for Qt6 find modules
set(CMAKE_PREFIX_PATH ${X86_LIB} ${X86_ROOT} ${CMAKE_PREFIX_PATH})
