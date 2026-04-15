/**
 * @file PlatformUtils.h
 * @brief 跨平台工具类 - 处理 Windows/Linux/macOS 平台差异
 * @author PrisonSIS Team
 * @version 1.0.0
 */
#ifndef PLATFORM_UTILS_H
#define PLATFORM_UTILS_H

#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QDir>

/**
 * @brief 跨平台工具类
 *
 * 提供跨平台的路径处理、系统信息获取等功能
 */
class PlatformUtils
{
public:
    /**
     * @brief 获取应用数据目录（跨平台）
     * @return 平台特定的应用数据目录路径
     *
     * Windows: C:/Users/<username>/AppData/Roaming/PrisonSystem
     * Linux:   ~/.local/share/PrisonSystem
     * macOS:   ~/Library/Application Support/PrisonSystem
     */
    static QString getAppDataDir();

    /**
     * @brief 获取备份目录（跨平台）
     * @return 平台特定的备份目录路径
     *
     * Windows: C:/Users/<username>/Documents/PrisonBackups
     * Linux:   ~/.local/share/PrisonSystem/backups
     * macOS:   ~/Library/Application Support/PrisonSystem/backups
     */
    static QString getBackupDir();

    /**
     * @brief 获取文档目录（跨平台）
     * @return 平台特定的文档目录路径
     */
    static QString getDocumentsDir();

    /**
     * @brief 获取桌面目录（跨平台）
     * @return 平台特定的桌面目录路径
     */
    static QString getDesktopDir();

    /**
     * @brief 安全清零内存（防止编译器优化）
     * @param data 要清零的字节数组
     *
     * Windows: 使用 SecureZeroMemory
     * Linux/macOS: 使用 volatile 指针防止优化
     */
    static void secureZeroMemory(QByteArray &data);

    /**
     * @brief 安全清零内存（通用版本）
     * @param ptr 内存指针
     * @param size 内存大小
     */
    static void secureZeroMemory(void *ptr, size_t size);

    /**
     * @brief 获取操作系统名称
     * @return "windows", "linux", "macos", "unknown"
     */
    static QString getOSName();

    /**
     * @brief 获取操作系统版本
     * @return 操作系统版本字符串
     */
    static QString getOSVersion();

    /**
     * @brief 获取计算机名
     * @return 计算机名
     */
    static QString getHostName();

    /**
     * @brief 获取用户名
     * @return 当前用户名
     */
    static QString getUserName();

    /**
     * @brief 检查是否为 Windows 平台
     * @return true if Windows
     */
    static bool isWindows();

    /**
     * @brief 检查是否为 Linux 平台
     * @return true if Linux
     */
    static bool isLinux();

    /**
     * @brief 检查是否为 macOS 平台
     * @return true if macOS
     */
    static bool isMacOS();

    /**
     * @brief 获取路径分隔符
     * @return "/" on Unix, "\\" on Windows
     */
    static QString getPathSeparator();

    /**
     * @brief 规范化路径（处理平台差异）
     * @param path 原始路径
     * @return 规范化后的路径
     */
    static QString normalizePath(const QString &path);

    /**
     * @brief 确保目录存在（跨平台）
     * @param path 目录路径
     * @return true if success
     */
    static bool ensureDirExists(const QString &path);

    /**
     * @brief 获取临时文件路径
     * @param prefix 文件名前缀
     * @return 临时文件路径
     */
    static QString getTempFilePath(const QString &prefix);

    /**
     * @brief 获取 OpenSSL 库文件名
     * @return OpenSSL DLL/SO 名称
     *
     * Windows: "libssl-3.dll", "libcrypto-3.dll"
     * Linux:  "libssl.so", "libcrypto.so"
     */
    static QStringList getOpenSSLLibraries();

    /**
     * @brief 获取 Qt 平台插件目录
     * @return Qt 插件目录路径
     */
    static QString getQtPluginsPath();

    /**
     * @brief 设置应用 DPI 感知（仅 Windows）
     * Windows Vista+ 默认启用 DPI 感知
     */
    static void setDPIAware();

    /**
     * @brief 初始化随机数生成器
     * 在应用启动时调用
     */
    static void initRandom();
};

#endif // PLATFORM_UTILS_H
