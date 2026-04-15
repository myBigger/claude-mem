#include "database/DatabaseManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QFile>
#include <QDir>
#include <QDebug>

DatabaseManager::DatabaseManager() {}
DatabaseManager::~DatabaseManager() { if (m_db.isOpen()) m_db.close(); }
DatabaseManager& DatabaseManager::instance() { static DatabaseManager i; return i; }

bool DatabaseManager::initDatabase(const QString& dbPath) {
    m_dbPath = dbPath;
    if (QSqlDatabase::contains("prison_record"))
        m_db = QSqlDatabase::database("prison_record");
    else
        m_db = QSqlDatabase::addDatabase("QSQLITE", "prison_record");
    m_db.setDatabaseName(dbPath);
    if (!m_db.open()) { qCritical() << "[DB] 打开失败:" << m_db.lastError().text(); return false; }
    if (!initSchema()) return false;
    seedDefaultData();
    return true;
}

bool DatabaseManager::initSchema() {
    QSqlQuery q(m_db);
    q.exec(R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id TEXT UNIQUE NOT NULL,
            username TEXT UNIQUE NOT NULL,
            password_hash TEXT NOT NULL,
            real_name TEXT,
            role TEXT NOT NULL DEFAULT 'Normal' CHECK(role IN ('Admin','Legal','Interrogator','ReadOnly')),
            department TEXT, position TEXT, phone TEXT,
            enabled INTEGER NOT NULL DEFAULT 1,
            failed_login_attempts INTEGER NOT NULL DEFAULT 0,
            locked_until TEXT,
            last_login_time TEXT,
            created_at TEXT NOT NULL DEFAULT (datetime('now')),
            remark TEXT
        )
    )");
    q.exec(R"(
        CREATE TABLE IF NOT EXISTS criminals (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            criminal_id TEXT UNIQUE NOT NULL,
            name TEXT NOT NULL, gender TEXT, ethnicity TEXT, birth_date TEXT,
            id_card_number TEXT UNIQUE, native_place TEXT, education TEXT,
            crime TEXT, sentence_years INTEGER DEFAULT 0, sentence_months INTEGER DEFAULT 0,
            entry_date TEXT, original_court TEXT, district TEXT, cell TEXT,
            crime_type TEXT, manage_level TEXT, handler_id TEXT, photo_path TEXT,
            remark TEXT, archived INTEGER NOT NULL DEFAULT 0,
            created_at TEXT NOT NULL DEFAULT (datetime('now')),
            updated_at TEXT NOT NULL DEFAULT (datetime('now'))
        )
    )");
    q.exec(R"(
        CREATE TABLE IF NOT EXISTS records (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            record_id TEXT UNIQUE NOT NULL,
            record_type TEXT NOT NULL,
            criminal_id INTEGER NOT NULL REFERENCES criminals(id),
            criminal_name TEXT,
            record_date TEXT, record_location TEXT,
            interrogator_id TEXT, recorder_id TEXT, present_persons TEXT,
            content TEXT,
            content_encrypted INTEGER NOT NULL DEFAULT 0,
            signed_interrogator INTEGER NOT NULL DEFAULT 0,
            signed_recorder INTEGER NOT NULL DEFAULT 0,
            signed_subject INTEGER NOT NULL DEFAULT 0,
            status TEXT NOT NULL DEFAULT 'Draft',
            approver1_id INTEGER, approver2_id INTEGER,
            approver1_opinion TEXT, approver2_opinion TEXT,
            approver1_result TEXT, approver2_result TEXT,
            reject_reason TEXT,
            version INTEGER NOT NULL DEFAULT 1,
            created_by INTEGER REFERENCES users(id),
            created_at TEXT NOT NULL DEFAULT (datetime('now')),
            updated_at TEXT NOT NULL DEFAULT (datetime('now')),
            scan_file_path TEXT,
            scan_hash TEXT,
            scan_signature TEXT,
            scan_file_size INTEGER,
            scan_uploaded_at TEXT,
            scan_uploaded_by INTEGER REFERENCES users(id)
        )
    )");
    // 扫描件字段（如果已存在则忽略，避免重复添加报错）
    q.exec("ALTER TABLE records ADD COLUMN scan_file_path TEXT");
    q.exec("ALTER TABLE records ADD COLUMN scan_hash TEXT");
    q.exec("ALTER TABLE records ADD COLUMN scan_signature TEXT");
    q.exec("ALTER TABLE records ADD COLUMN scan_file_size INTEGER");
    q.exec("ALTER TABLE records ADD COLUMN scan_uploaded_at TEXT");
    q.exec("ALTER TABLE records ADD COLUMN scan_uploaded_by INTEGER");
    q.exec("ALTER TABLE records ADD COLUMN content_encrypted INTEGER NOT NULL DEFAULT 0");
    q.exec(R"(
        CREATE TABLE IF NOT EXISTS approvals (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            record_id INTEGER NOT NULL REFERENCES records(id),
            approval_level INTEGER NOT NULL,
            approver_id INTEGER REFERENCES users(id),
            result TEXT NOT NULL CHECK(result IN ('Pending','Approved','Rejected','Withdrawn')),
            opinion TEXT, reject_reason TEXT, approval_time TEXT,
            created_at TEXT NOT NULL DEFAULT (datetime('now'))
        )
    )");
    q.exec(R"(
        CREATE TABLE IF NOT EXISTS templates (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            template_id TEXT UNIQUE NOT NULL,
            name TEXT NOT NULL, record_type TEXT NOT NULL,
            version INTEGER NOT NULL DEFAULT 1,
            content TEXT NOT NULL,
            status TEXT NOT NULL DEFAULT 'Enabled',
            created_by INTEGER REFERENCES users(id),
            created_at TEXT NOT NULL DEFAULT (datetime('now')),
            updated_at TEXT NOT NULL DEFAULT (datetime('now'))
        )
    )");
    q.exec(R"(
        CREATE TABLE IF NOT EXISTS audit_logs (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER REFERENCES users(id), username TEXT,
            action TEXT NOT NULL, module TEXT NOT NULL,
            target_type TEXT, target_id INTEGER, details TEXT,
            ip_address TEXT DEFAULT '127.0.0.1',
            timestamp TEXT NOT NULL DEFAULT (datetime('now'))
        )
    )");
    q.exec(R"(CREATE VIRTUAL TABLE IF NOT EXISTS records_fts USING fts5(
        record_id, criminal_name, content, content=records, content_rowid=id))");
    q.exec(R"(CREATE TRIGGER IF NOT EXISTS records_ai AFTER INSERT ON records BEGIN
        INSERT INTO records_fts(rowid, record_id, criminal_name, content)
        VALUES (new.id, new.record_id, '', new.content); END)");
    q.exec(R"(CREATE TRIGGER IF NOT EXISTS records_au AFTER UPDATE ON records BEGIN
        UPDATE records_fts SET content = new.content WHERE rowid = new.id; END)");
    qDebug() << "[DB] 表结构初始化完成";
    return true;
}

bool DatabaseManager::seedDefaultData() {
    QSqlQuery q(m_db);
    q.exec("SELECT COUNT(*) FROM users WHERE role='Admin'");
    q.next();
    if (q.value(0).toInt() > 0) return true;

    // 逐条插入用户，避免多行 INSERT 在部分 SQLite 版本/驱动上兼容性问题
    // role 必须符合 CHECK(role IN ('Admin','Legal','Interrogator','ReadOnly'))
    // Normal 不在允许列表，故改为 ReadOnly
    QStringList usersSql = {
        "INSERT INTO users (user_id,username,password_hash,real_name,role,department,position,phone,enabled,failed_login_attempts,locked_until,last_login_time,created_at,remark) VALUES ('US-0001','admin','2261ae7039c889223f6653792a4d2da8','系统管理员','Admin','信息科','科长','',1,0,'','','2026-01-01 00:00:00','')",
        "INSERT INTO users (user_id,username,password_hash,real_name,role,department,position,phone,enabled,failed_login_attempts,locked_until,last_login_time,created_at,remark) VALUES ('US-0002','user01','d05e67e42bcf4e2c343c1106e0527a6e','张三','ReadOnly','一监区','管教员','',1,0,'','','2026-01-01 00:00:00','')",
        "INSERT INTO users (user_id,username,password_hash,real_name,role,department,position,phone,enabled,failed_login_attempts,locked_until,last_login_time,created_at,remark) VALUES ('US-0003','user02','11290d4bd041ac0268f914af0eaac72c','李四','ReadOnly','二监区','管教员','',1,0,'','','2026-01-01 00:00:00','')",
    };
    for (const QString& sql : usersSql) {
        if (!q.exec(sql)) {
            qCritical() << "[DB] 用户插入失败:" << q.lastError().text() << "SQL:" << sql;
        }
    }
    QStringList tpls = {
        "TM-01|入监笔录标准模板|RT-01|1|浙江省[监狱名称]入监笔录\n\n讯问人：[___讯问人___]  记录员：[___记录员___]\n审讯时间：[___YYYY-MM-DD HH:MM___]  地点：[___地点___]\n\n一、基本情况\n★讯问人签字：[___________]  ★记录员签字：[___________]  ★被讯问人签字：[___________]",
        "TM-02|日常谈话标准模板|RT-02|1|浙江省[监狱名称]日常谈话笔录\n\n讯问人：[___讯问人___]  记录员：[___记录员___]\n审讯时间：[___YYYY-MM-DD HH:MM___]  地点：[___地点___]\n\n★讯问人签字：[___________]  ★记录员签字：[___________]  ★被讯问人签字：[___________]",
        "TM-03|案件调查标准模板|RT-03|1|浙江省[监狱名称]案件调查笔录\n\n讯问人：[___讯问人___]  记录员：[___记录员___]\n审讯时间：[___YYYY-MM-DD HH:MM___]  地点：[___地点___]\n\n★讯问人签字：[___________]  ★记录员签字：[___________]  ★被讯问人签字：[___________]",
        "TM-04|行政处罚告知标准模板|RT-04|1|浙江省[监狱名称]行政处罚告知笔录\n\n讯问人：[___讯问人___]  记录员：[___记录员___]\n审讯时间：[___YYYY-MM-DD HH:MM___]  地点：[___地点___]\n\n★讯问人签字：[___________]  ★记录员签字：[___________]  ★被讯问人签字：[___________]",
        "TM-05|减刑假释评估标准模板|RT-05|1|浙江省[监狱名称]减刑假释评估笔录\n\n讯问人：[___讯问人___]  记录员：[___记录员___]\n审讯时间：[___YYYY-MM-DD HH:MM___]  地点：[___地点___]\n\n★讯问人签字：[___________]  ★记录员签字：[___________]  ★被讯问人签字：[___________]",
        "TM-06|出监教育标准模板|RT-06|1|浙江省[监狱名称]出监教育笔录\n\n讯问人：[___讯问人___]  记录员：[___记录员___]\n审讯时间：[___YYYY-MM-DD HH:MM___]  地点：[___地点___]\n\n★讯问人签字：[___________]  ★记录员签字：[___________]  ★被讯问人签字：[___________]",
        "TM-07|医疗检查标准模板|RT-07|1|浙江省[监狱名称]医疗检查笔录\n\n讯问人：[___讯问人___]  记录员：[___记录员___]\n审讯时间：[___YYYY-MM-DD HH:MM___]  地点：[___地点___]\n\n★讯问人签字：[___________]  ★记录员签字：[___________]  ★被讯问人签字：[___________]",
        "TM-08|亲属会见标准模板|RT-08|1|浙江省[监狱名称]亲属会见笔录\n\n讯问人：[___讯问人___]  记录员：[___记录员___]\n审讯时间：[___YYYY-MM-DD HH:MM___]  地点：[___地点___]\n\n★讯问人签字：[___________]  ★记录员签字：[___________]  ★被讯问人签字：[___________]"
    };
    for (const QString& t : tpls) {
        QStringList p = t.split('|');
        if (p.size() >= 5) {
            QSqlQuery iq(m_db);
            iq.prepare(R"(INSERT OR IGNORE INTO templates VALUES (NULL,?,?,'Normal',?,?,'Enabled',1,'2026-01-01 00:00:00','2026-01-01 00:00:00'))");
            iq.addBindValue(p[0]); iq.addBindValue(p[1]); iq.addBindValue(p[2]);
            iq.addBindValue(p[3].toInt()); iq.addBindValue(p[4]);
            iq.exec();
        }
    }
    qDebug() << "[DB] 默认数据填充完成，登录：admin/admin user01/user01 user02/user02";
    return true;
}

QSqlQuery DatabaseManager::query(const QString& sql, const QMap<QString,QVariant>& params) {
    QSqlQuery q(m_db);
    q.prepare(sql);
    for (auto it = params.constBegin(); it != params.constEnd(); ++it)
        q.bindValue(":" + it.key(), it.value());
    q.exec();
    return q;
}

int DatabaseManager::insert(const QString& table, const QMap<QString,QVariant>& values) {
    QStringList keys, placeholders;
    for (auto it = values.constBegin(); it != values.constEnd(); ++it) {
        keys << it.key(); placeholders << ":" + it.key();
    }
    QSqlQuery q(m_db);
    q.prepare(QString("INSERT INTO %1 (%2) VALUES (%3)").arg(table).arg(keys.join(",")).arg(placeholders.join(",")));
    for (auto it = values.constBegin(); it != values.constEnd(); ++it)
        q.bindValue(":" + it.key(), it.value());
    if (q.exec()) return q.lastInsertId().toInt();
    qCritical() << "[DB] INSERT失败:" << q.lastError().text();
    return -1;
}

bool DatabaseManager::update(const QString& table, const QMap<QString,QVariant>& values,
                              const QString& where, const QMap<QString,QVariant>& whereParams) {
    QStringList sets;
    for (auto it = values.constBegin(); it != values.constEnd(); ++it)
        sets << it.key() + " = :" + it.key();
    QSqlQuery q(m_db);
    q.prepare(QString("UPDATE %1 SET %2 WHERE %3").arg(table).arg(sets.join(",")).arg(where));
    for (auto it = values.constBegin(); it != values.constEnd(); ++it)
        q.bindValue(":" + it.key(), it.value());
    for (auto it = whereParams.constBegin(); it != whereParams.constEnd(); ++it)
        q.bindValue(":" + it.key(), it.value());
    return q.exec();
}

bool DatabaseManager::remove(const QString& table, const QString& where,
                              const QMap<QString,QVariant>& whereParams) {
    QSqlQuery q(m_db);
    q.prepare(QString("DELETE FROM %1 WHERE %2").arg(table).arg(where));
    for (auto it = whereParams.constBegin(); it != whereParams.constEnd(); ++it)
        q.bindValue(":" + it.key(), it.value());
    return q.exec();
}

QVector<QMap<QString,QVariant>> DatabaseManager::select(const QString& sql,
                                                        const QMap<QString,QVariant>& params) {
    QVector<QMap<QString,QVariant>> results;
    QSqlQuery q(m_db);
    q.prepare(sql);
    for (auto it = params.constBegin(); it != params.constEnd(); ++it)
        q.bindValue(":" + it.key(), it.value());
    q.exec();
    while (q.next()) {
        QMap<QString,QVariant> row;
        QSqlRecord rec = q.record();
        for (int i = 0; i < rec.count(); ++i)
            row[rec.fieldName(i)] = rec.value(i);
        results.append(row);
    }
    return results;
}

QMap<QString,QVariant> DatabaseManager::selectOne(const QString& sql,
                                                  const QMap<QString,QVariant>& params) {
    auto r = select(sql, params);
    return r.isEmpty() ? QMap<QString,QVariant>() : r.first();
}

bool DatabaseManager::backup(const QString& path) {
    m_db.close();
    bool ok = QFile::copy(m_dbPath, path);
    m_db.open();
    return ok;
}

bool DatabaseManager::restore(const QString& path) {
    if (!QFile::exists(path)) return false;
    m_db.close();
    QFile::remove(m_dbPath);
    bool ok = QFile::copy(path, m_dbPath);
    m_db.open();
    return ok;
}
