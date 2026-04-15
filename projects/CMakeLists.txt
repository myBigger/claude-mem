cmake_minimum_required(VERSION 3.20)
project(PrisonRecordTool VERSION 1.0.0 LANGUAGES CXX)

# ============================================
# 跨平台配置
# ============================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC OFF)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Windows 特定配置
if(WIN32)
    #Qt6 的安装路径
    if(NOT CMAKE_PREFIX_PATH)
        set(CMAKE_PREFIX_PATH "$ENV{QT6_ROOT}")
    endif()
    #使用 Qt 自带的 OpenSSL
    if(NOT OPENSSL_ROOT_DIR)
        if(DEFINED ENV{OPENSSL_ROOT})
            set(OPENSSL_ROOT_DIR "$ENV{OPENSSL_ROOT}")
        else()
            set(OPENSSL_ROOT_DIR "C:/Program Files/OpenSSL-Win64")
        endif()
    endif()
    # 禁用一些 MSVC 警告
    add_compile_options(/W3 /utf-8)
    # 使用 Unicode
    add_definitions(-DUNICODE -D_UNICODE)
    # 支持 Windows XP 及以上
    add_definitions(-DWINVER=0x0601 -D_WIN32_WINNT=0x0601)
    # 静态链接 Qt 运行时
    set(CMAKE_QT_IMPORTIONS_TARGET ${PROJECT_NAME})
endif()

# 查找依赖
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Sql Xml PrintSupport Network)

# OpenSSL 配置
find_package(OpenSSL REQUIRED)

# 输出目录配置（跨平台）
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Windows 下复制依赖 DLL 到输出目录
if(WIN32)
    # Qt 插件目录
    set(QT_PLUGINS_DIR ${Qt6_DIR}/../../../plugins)
    if(EXISTS "${QT_PLUGINS_DIR}")
        message(STATUS "Qt plugins: ${QT_PLUGINS_DIR}")
    endif()
endif()

set(MOC_HEADERS
    include/ui/MainWindow.h
    include/ui/LoginDialog.h
    include/ui/PINDialog.h
    include/ui/widgets/CaseListWidget.h
    include/ui/widgets/CaseDetailWidget.h
    include/ui/widgets/CriminalListWidget.h
    include/ui/widgets/CriminalDetailWidget.h
    include/ui/widgets/CriminalEditDialog.h
    include/ui/widgets/CriminalFetchDialog.h
    include/ui/widgets/RecordEditorWidget.h
    include/ui/widgets/LinedTextEdit.h
    include/ui/widgets/RecordThemeSelector.h
    include/ui/widgets/StatisticsWidget.h
    include/ui/widgets/ApprovalWidget.h
    include/ui/widgets/ArchiveWidget.h
    include/ui/widgets/TemplateWidget.h
    include/ui/widgets/UserManageWidget.h
    include/ui/widgets/BackupWidget.h
    include/ui/widgets/LogViewerWidget.h
    include/ui/widgets/ScanUploadDialog.h
    include/ui/widgets/ScanDownloadDialog.h
    include/services/AuthService.h
    include/services/AuditService.h
    include/services/PrisonApiService.h
    include/services/CryptoService.h
    include/services/P2PNodeService.h
    include/services/KeyManager.h
)
qt6_wrap_cpp(MOC_SOURCES ${MOC_HEADERS})

set(HEADERS
    include/models/User.h include/models/Criminal.h include/models/Record.h
    include/models/Approval.h include/models/Template.h include/models/AuditLog.h
    include/models/Case.h
    include/database/DatabaseManager.h
    include/utils/PasswordHasher.h include/utils/DateUtils.h include/utils/EncryptUtil.h
    include/utils/PlatformUtils.h
    include/services/AuthService.h include/services/AuditService.h
    include/services/PrisonApiService.h include/services/CryptoService.h
    include/services/P2PNodeService.h
    include/ui/MainWindow.h include/ui/LoginDialog.h
    include/ui/widgets/CaseListWidget.h include/ui/widgets/CaseDetailWidget.h
    include/ui/widgets/CriminalListWidget.h include/ui/widgets/CriminalDetailWidget.h
    include/ui/widgets/CriminalEditDialog.h include/ui/widgets/CriminalFetchDialog.h
    include/ui/widgets/RecordEditorWidget.h include/ui/widgets/LinedTextEdit.h
    include/ui/widgets/RecordThemeSelector.h include/ui/widgets/StatisticsWidget.h
    include/ui/widgets/ApprovalWidget.h include/ui/widgets/ArchiveWidget.h
    include/ui/widgets/TemplateWidget.h include/ui/widgets/UserManageWidget.h
    include/ui/widgets/BackupWidget.h include/ui/widgets/LogViewerWidget.h
    include/ui/widgets/ScanUploadDialog.h
    include/ui/widgets/ScanDownloadDialog.h
)

set(SOURCES
    src/main.cpp
    src/models/User.cpp src/models/Criminal.cpp src/models/Record.cpp
    src/models/Approval.cpp src/models/Template.cpp src/models/AuditLog.cpp
    src/models/Case.cpp
    src/database/DatabaseManager.cpp
    src/services/AuthService.cpp src/services/AuditService.cpp
    src/services/PrisonApiService.cpp src/services/CryptoService.cpp
    src/services/P2PNodeService.cpp src/services/KeyManager.cpp
    src/utils/PasswordHasher.cpp src/utils/DateUtils.cpp src/utils/EncryptUtil.cpp
    src/utils/PlatformUtils.cpp
    src/ui/MainWindow.cpp src/ui/LoginDialog.cpp src/ui/PINDialog.cpp
    src/ui/widgets/CaseListWidget.cpp src/ui/widgets/CaseDetailWidget.cpp
    src/ui/widgets/CriminalListWidget.cpp src/ui/widgets/CriminalDetailWidget.cpp
    src/ui/widgets/CriminalEditDialog.cpp src/ui/widgets/CriminalFetchDialog.cpp
    src/ui/widgets/RecordEditorWidget.cpp src/ui/widgets/LinedTextEdit.cpp
    src/ui/widgets/RecordThemeSelector.cpp src/ui/widgets/StatisticsWidget.cpp
    src/ui/widgets/ApprovalWidget.cpp src/ui/widgets/ArchiveWidget.cpp
    src/ui/widgets/TemplateWidget.cpp src/ui/widgets/UserManageWidget.cpp
    src/ui/widgets/BackupWidget.cpp src/ui/widgets/LogViewerWidget.cpp
    src/ui/widgets/ScanUploadDialog.cpp
    src/ui/widgets/ScanDownloadDialog.cpp
)

set(RESOURCES resources/icons.qrc)
add_executable(${PROJECT_NAME} WIN32 ${HEADERS} ${SOURCES} ${MOC_SOURCES} ${RESOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Sql Qt6::Xml Qt6::PrintSupport Qt6::Network
    OpenSSL::SSL OpenSSL::Crypto
)
if(WIN32)
    # Windows 特定库链接
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ws2_32     # Winsock2 (P2P 网络)
        advapi32   # Windows API (安全功能)
        user32     # Windows 用户界面
    )

    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endif()

# ============================================
# PrisonCLI — 命令行工具
# ============================================
set(CLI_SOURCES
    src/cli/main.cpp
    src/cli/cli_utils.cpp
    src/cli/formatter.cpp
    src/cli/commands/criminals_cmd.cpp
    src/cli/commands/records_cmd.cpp
    src/cli/commands/approval_cmd.cpp
    src/cli/commands/audit_cmd.cpp
    src/cli/commands/export_cmd.cpp
)

# CLI 可执行文件（不依赖 GUI 组件）
add_executable(prison-cli
    ${CLI_SOURCES}
    src/models/User.cpp
    src/models/Criminal.cpp
    src/models/Record.cpp
    src/models/Approval.cpp
    src/models/Template.cpp
    src/models/AuditLog.cpp
    src/models/Case.cpp
    src/database/DatabaseManager.cpp
    src/services/AuthService.cpp
    src/services/AuditService.cpp
    src/services/CryptoService.cpp
    src/services/KeyManager.cpp
    src/utils/PasswordHasher.cpp
    src/utils/DateUtils.cpp
    src/utils/EncryptUtil.cpp
    src/utils/PlatformUtils.cpp
)

target_include_directories(prison-cli PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(prison-cli PRIVATE
    Qt6::Core Qt6::Sql Qt6::Network
    OpenSSL::SSL OpenSSL::Crypto
)
if(WIN32)
    target_link_libraries(prison-cli PRIVATE ws2_32 advapi32)
endif()

# ============================================
# 安装配置（跨平台）
# ============================================
if(WIN32)
    # Windows 安装路径
    set(CMAKE_INSTALL_PREFIX "C:/Program Files/PrisonRecordTool")
    install(TARGETS ${PROJECT_NAME} prison-cli
        RUNTIME DESTINATION bin
        COMPONENT Runtime)
else()
    # Linux/macOS 安装路径
    set(CMAKE_INSTALL_PREFIX "/usr/local")
    install(TARGETS ${PROJECT_NAME} prison-cli
        RUNTIME DESTINATION bin
        COMPONENT Runtime)
endif()

# 输出编译信息
message(STATUS "=============================================")
message(STATUS "PrisonRecordTool v1.0.0")
message(STATUS "  Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Qt6: ${Qt6_VERSION}")
message(STATUS "  OpenSSL: ${OPENSSL_VERSION}")
message(STATUS "  C++ Standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "  P2P Enabled: YES")
message(STATUS "=============================================")
