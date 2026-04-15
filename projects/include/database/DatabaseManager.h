#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H
#include <QString>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QVariant>
#include <QMap>
#include <QVector>

class DatabaseManager {
public:
    static DatabaseManager& instance();
    bool initDatabase(const QString& dbPath);
    bool initSchema();
    bool seedDefaultData();
    QSqlDatabase database() const { return m_db; }
    QSqlQuery query(const QString& sql, const QMap<QString,QVariant>& params = {});
    int insert(const QString& table, const QMap<QString,QVariant>& values);
    bool update(const QString& table, const QMap<QString,QVariant>& values,
                const QString& where, const QMap<QString,QVariant>& whereParams);
    bool remove(const QString& table, const QString& where,
                const QMap<QString,QVariant>& whereParams);
    QVector<QMap<QString,QVariant>> select(const QString& sql,
                                           const QMap<QString,QVariant>& params = {});
    QMap<QString,QVariant> selectOne(const QString& sql,
                                      const QMap<QString,QVariant>& params = {});
    bool backup(const QString& path);
    bool restore(const QString& path);
    QString databasePath() const { return m_dbPath; }
private:
    DatabaseManager();
    ~DatabaseManager();
    DatabaseManager(const DatabaseManager&) = delete;
    QSqlDatabase m_db;
    QString m_dbPath;
};
#endif
