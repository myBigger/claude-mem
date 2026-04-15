/**
 * @file PlatformUtils.cpp
 * @brief 跨平台工具类实现
 */
#include "utils/PlatformUtils.h"
#include <QCoreApplication>
#include <QStandardPaths>
#include <QDateTime>
#include <QFile>
#include <QDir>
#include <QSysInfo>
#include <QLibraryInfo>

#ifdef Q_OS_WIN
#include <windows.h>
#include <wincrypt.h>
#pragma comment(lib, "advapi32.lib")
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

// ============================================
// 路径处理
// ============================================
QString PlatformUtils::getAppDataDir()
{
    QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    // 确保目录存在
    QDir().mkpath(base);
    return base;
}

QString PlatformUtils::getBackupDir()
{
    QString backupDir;
    if (isWindows()) {
        // Windows: 使用文档目录
        backupDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                   + "/PrisonBackups";
    } else {
        // Linux/macOS: 使用 AppData 目录
        backupDir = getAppDataDir() + "/backups";
    }
    QDir().mkpath(backupDir);
    return backupDir;
}

QString PlatformUtils::getDocumentsDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
}

QString PlatformUtils::getDesktopDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
}

// ============================================
// 安全内存清零
// ============================================
void PlatformUtils::secureZeroMemory(QByteArray &data)
{
    if (data.isEmpty()) return;
    secureZeroMemory(data.data(), static_cast<size_t>(data.size()));
    data.clear();
}

void PlatformUtils::secureZeroMemory(void *ptr, size_t size)
{
    if (!ptr || size == 0) return;

#ifdef Q_OS_WIN
    // Windows: 使用 SecureZeroMemory（不会被编译器优化掉）
    SecureZeroMemory(ptr, size);
#else
    // Linux/macOS: 使用 volatile 指针防止编译器优化
    volatile unsigned char *p = static_cast<volatile unsigned char *>(ptr);
    while (size--) {
        *p++ = 0;
    }
#endif
}

// ============================================
// 系统信息
// ============================================
QString PlatformUtils::getOSName()
{
#ifdef Q_OS_WIN
    return "windows";
#elif defined(Q_OS_MAC)
    return "macos";
#elif defined(Q_OS_LINUX)
    return "linux";
#else
    return "unknown";
#endif
}

QString PlatformUtils::getOSVersion()
{
    return QSysInfo::productVersion();
}

QString PlatformUtils::getHostName()
{
#ifdef Q_OS_WIN
    wchar_t hostname[256];
    DWORD size = 256;
    if (GetComputerNameW(hostname, &size)) {
        return QString::fromWCharArray(hostname);
    }
#else
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        return QString::fromLatin1(hostname);
    }
#endif
    return "unknown";
}

QString PlatformUtils::getUserName()
{
#ifdef Q_OS_WIN
    wchar_t username[256];
    DWORD size = 256;
    if (GetUserNameW(username, &size)) {
        return QString::fromWCharArray(username);
    }
#else
    const char *user = getenv("USER");
    if (!user) user = getenv("USERNAME");
    if (user) return QString::fromLatin1(user);
#endif
    return "unknown";
}

bool PlatformUtils::isWindows()
{
#ifdef Q_OS_WIN
    return true;
#else
    return false;
#endif
}

bool PlatformUtils::isLinux()
{
#ifdef Q_OS_LINUX
    return true;
#else
    return false;
#endif
}

bool PlatformUtils::isMacOS()
{
#ifdef Q_OS_MAC
    return true;
#else
    return false;
#endif
}

// ============================================
// 路径处理
// ============================================
QString PlatformUtils::getPathSeparator()
{
    return QDir::separator();
}

QString PlatformUtils::normalizePath(const QString &path)
{
    QString result = QDir::toNativeSeparators(path);
    // 移除多余的斜杠
    result.replace("//", "/");
    return result;
}

bool PlatformUtils::ensureDirExists(const QString &path)
{
    QDir dir(path);
    if (dir.exists()) return true;
    return dir.mkpath(path);
}

QString PlatformUtils::getTempFilePath(const QString &prefix)
{
    QString tempPath = QDir::tempPath();
    QString fileName = prefix + "_" + QString::number(QDateTime::currentMSecsSinceEpoch());
    return tempPath + getPathSeparator() + fileName;
}

// ============================================
// OpenSSL 库
// ============================================
QStringList PlatformUtils::getOpenSSLLibraries()
{
    if (isWindows()) {
        return QStringList() << "libssl-3.dll" << "libcrypto-3-x64.dll";
    } else if (isMacOS()) {
        return QStringList() << "libssl.3.dylib" << "libcrypto.3.dylib";
    } else {
        return QStringList() << "libssl.so" << "libcrypto.so";
    }
}

// ============================================
// Qt 配置
// ============================================
QString PlatformUtils::getQtPluginsPath()
{
    return QLibraryInfo::path(QLibraryInfo::PluginsPath);
}

// ============================================
// DPI 和其他系统配置
// ============================================
void PlatformUtils::setDPIAware()
{
#ifdef Q_OS_WIN
    // Windows Vista 及以上启用 DPI 感知
    // SetProcessDPIAware 在 Windows 8.1+ 应该使用 SetProcessDpiAwareness
    typedef BOOL(WINAPI * SetProcessDPIAwareFunc)(void);
    typedef HRESULT(WINAPI * SetProcessDpiAwarenessFunc)(int);

    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (user32) {
        SetProcessDpiAwarenessFunc setDpiAwareness =
            (SetProcessDpiAwarenessFunc)GetProcAddress(user32, "SetProcessDpiAwareness");
        if (setDpiAwareness) {
            // DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 = -4
            setDpiAwareness(2); // PROCESS_PER_MONITOR_DPI_AWARE
        } else {
            SetProcessDPIAwareFunc setDPIAware =
                (SetProcessDPIAwareFunc)GetProcAddress(user32, "SetProcessDPIAware");
            if (setDPIAware) {
                setDPIAware();
            }
        }
    }
#endif
}

// ============================================
// 随机数初始化
// ============================================
void PlatformUtils::initRandom()
{
#ifdef Q_OS_WIN
    // Windows: 使用 CryptGenRandom
    HCRYPTPROV hCryptProv;
    if (CryptAcquireContextW(&hCryptProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        unsigned char randomBytes[32];
        CryptGenRandom(hCryptProv, sizeof(randomBytes), randomBytes);
        CryptReleaseContext(hCryptProv, 0);
    }
#else
    // Unix: 读取 /dev/urandom 初始化
    QFile urandom("/dev/urandom");
    if (urandom.open(QIODevice::ReadOnly)) {
        char randomBytes[32];
        urandom.read(randomBytes, sizeof(randomBytes));
        urandom.close();
    }
#endif
}
