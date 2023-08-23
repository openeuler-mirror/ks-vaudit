#include "sql-configure.h"
#include <QFile>
#include <QDebug>
#include <QRegExp>
#include <QSqlError>
#include <QSqlQuery>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QMutexLocker>
#include <QByteArray>

//#define SUPPOORT_SQLITECIPHER

#define SQLITE_NAME             "ks-vaudit"
#define SQLITE_PATH             "/etc/ks-vaudit.sqlite"
#define PARAM_USER              "user"
#define PARAM_USER_PASSWD       "passwd"
#define PARAM_USER_ROLE         "role"
#define PARAM_USER_OLD_PASSWD   "old_passwd"
#define PARAM_DB_PASSWD         "db_passwd"

SQLConfigure &SQLConfigure::Instance()
{
    static SQLConfigure s_sql;
    return s_sql;
}

bool SQLConfigure::createUser(const QString param)
{
    QString str = param;
    QJsonDocument jsonDocument = QJsonDocument::fromJson(str.toLatin1());
    if (!jsonDocument.isObject())
        return false;


    QJsonObject jsonObj = jsonDocument.object();
    if (!jsonObj.contains(PARAM_USER) || !jsonObj.contains(PARAM_USER_PASSWD) || !jsonObj.contains(PARAM_USER_ROLE))
        return false;

    return createUser(jsonObj[PARAM_USER].toString(), jsonObj[PARAM_USER_PASSWD].toString(), jsonObj[PARAM_USER_ROLE].toString());
}

bool SQLConfigure::deleteUser(const QString param)
{
    QString str = param;
    QJsonDocument jsonDocument = QJsonDocument::fromJson(str.toLatin1());
    if (!jsonDocument.isObject())
        return false;

    QJsonObject jsonObj = jsonDocument.object();
    if (!jsonObj.contains(PARAM_USER) || !jsonObj.contains(PARAM_USER_PASSWD))
        return false;

    return deleteUser(jsonObj[PARAM_USER].toString(), jsonObj[PARAM_USER_PASSWD].toString());
}

bool SQLConfigure::updateUser(const QString param)
{
    QString str = param;
    QJsonDocument jsonDocument = QJsonDocument::fromJson(str.toLatin1());
    if (!jsonDocument.isObject())
        return false;

    QJsonObject jsonObj = jsonDocument.object();
    if (!jsonObj.contains(PARAM_USER) || !jsonObj.contains(PARAM_USER_OLD_PASSWD))
        return false;

    QString newpwd{}, role{};
    if (jsonObj.contains(PARAM_USER_PASSWD))
        newpwd = jsonObj[PARAM_USER_PASSWD].toString();
    if (jsonObj.contains(PARAM_USER_ROLE))
        role = jsonObj[PARAM_USER_ROLE].toString();

    return updateUser(jsonObj[PARAM_USER].toString(), jsonObj[PARAM_USER_OLD_PASSWD].toString(), newpwd, role);
}

QString SQLConfigure::queryUser(const QString param)
{
    QString str = param;
    QJsonDocument jsonDocument = QJsonDocument::fromJson(str.toLatin1());
    if (!jsonDocument.isObject())
        return "";

    QJsonObject jsonObj = jsonDocument.object();
    if (!jsonObj.contains(PARAM_DB_PASSWD))
        return "";

    QString usr{};
    if (jsonObj.contains(PARAM_USER))
        usr = jsonObj[PARAM_USER].toString();

    return queryUser(usr, jsonObj[PARAM_DB_PASSWD].toString());
}

SQLConfigure::SQLConfigure()
{
    m_usr = "vaudit";
    m_pwd = "12345678";
    qWarning() << __func__ << "sqldrivers: " << QSqlDatabase::drivers();
    initDB();
}

SQLConfigure::~SQLConfigure()
{
    if (m_db.isOpen())
        m_db.close();

    if (m_query)
    {
        delete m_query;
        m_query = nullptr;
    }

    QSqlDatabase::removeDatabase(SQLITE_NAME);
}

bool SQLConfigure::initDB()
{
    QMutexLocker locker(&m_mutex);
#ifdef SUPPOORT_SQLITECIPHER
    m_db = QSqlDatabase::addDatabase("SQLITECIPHER", SQLITE_NAME);
    m_db.setDatabaseName(SQLITE_PATH);
    m_db.setUserName(m_usr);
    m_db.setPassword(m_pwd);

    QFile file(SQLITE_PATH);
    if (file.exists())
        m_db.setConnectOptions("QSQLITE_USE_CIPHER=sqlcipher");
    else
        m_db.setConnectOptions("QSQLITE_CREATE_KEY;QSQLITE_USE_CIPHER=sqlcipher");
#else
    m_db = QSqlDatabase::addDatabase("QSQLITE", SQLITE_PATH);
    m_db.setDatabaseName(SQLITE_PATH);
#endif
    if (!m_db.open())
    {
        qWarning()<<"db open failed:"<<m_db.lastError().text()<< "db name :"<< SQLITE_NAME << m_usr << m_pwd;
        return false;
    }

    QString str = "PRAGMA foreign_keys = ON";
    m_query = new QSqlQuery(m_db);
    if (!m_query->exec(str))
    {
        qWarning()<<"db foreign_keys failed:"<<m_db.lastError().text();
        return false;
    }

    m_query->exec(QString("select count(*) from sqlite_master where type='table' and name='user_infos'"));
    if (m_query->next())
    {
        if (m_query->value(0).toInt()==0)
        {
            QString create_sql = "create table user_infos (id integer PRIMARY KEY AUTOINCREMENT, name varchar(255) NOT NULL DEFAULT '' UNIQUE,\
                    role varchar(255) NOT NULL DEFAULT '', password varchar(255) NOT NULL DEFAULT '')";

            m_query->prepare(create_sql);
            if (!m_query->exec())
            {
                qWarning() << "Error: Fail to create table." << m_query->lastError();
            }
        }
    }

#if 0
    m_query->exec(QString("select count(*) from sqlite_master where type='table' and name='raw_data'"));
    if (m_query->next())
    {
        if (m_query->value(0).toInt()==0)
        {
            QString create_sql = "create table raw_data (id integer PRIMARY KEY AUTOINCREMENT, path varchar(255) NOT NULL DEFAULT '' UNIQUE, starttime integer(20) NOT NULL DEFAULT 0,\
                    endtime integer(20) NOT NULL DEFAULT 0, fps INT(20) NOT NULL DEFAULT 0, user varchar(255) NOT NULL DEFAULT '', ip varchar(255) NOT NULL DEFAULT '')";

            m_query->prepare(create_sql);
            if (!m_query->exec())
            {
                qWarning() << "Error: Fail to create table." << m_query->lastError();
            }
        }
    }
#endif
    return true;
}

bool SQLConfigure::checkUserPasswd(const QString name, const QString pwd)
{
    QString sql = "select * from user_infos where name = ? and password = ?";
    m_query->prepare(sql);
    m_query->addBindValue(name);
    QByteArray byteArr(pwd.toStdString().data());
    m_query->addBindValue(byteArr.toBase64());
    if (!m_query->exec())
    {
        qWarning() <<  __func__ << __LINE__  << "query user " << name << " failed: " << m_query->lastError();
        return false;
    }

    while (m_query->next())
        return true;

    qWarning() << __func__ << __LINE__ << "user or passwd err";
    return false;
}

bool SQLConfigure::isNameLegal(QString name)
{
    QRegExp rxp("^\\w{1,32}$");
    if (rxp.exactMatch(name))
        return true;

    qWarning() << name << " is illegal";
    return false;
}

bool SQLConfigure::createUser(const QString name, const QString pwd, const QString role)
{
    if (name.isEmpty() || pwd.isEmpty() || role.isEmpty())
        return false;

    if (!isNameLegal(name))
        return false;

    QMutexLocker locker(&m_mutex);
    QString sql = "insert into user_infos values (null, ?, ?, ?)";
    m_query->prepare(sql);
    m_query->addBindValue(name);
    m_query->addBindValue(role);
    QByteArray byteArr(pwd.toStdString().data());
    m_query->addBindValue(byteArr.toBase64());

    if (!m_query->exec())
    {
        qWarning() << "insert user " << name << " failed: " << m_query->lastError().text();
        return false;
    }

    return true;
}

bool SQLConfigure::deleteUser(const QString name, const QString pwd)
{
    QMutexLocker locker(&m_mutex);
    if (!checkUserPasswd(name, pwd))
        return false;

    QString sql = "delete from user_infos where name = ?";
    m_query->prepare(sql);
    m_query->addBindValue(name);
    if (!m_query->exec())
    {
        qWarning() << "delete user " << name << " failed: " << m_query->lastError().text();
        return false;
    }

    return true;
}

bool SQLConfigure::updateUser(const QString name, const QString oldpwd, const QString newpwd, const QString role)
{
    QMutexLocker locker(&m_mutex);
    if (!checkUserPasswd(name, oldpwd))
        return false;

    if (newpwd.isEmpty() && role.isEmpty())
        return false;
    else if (newpwd.isEmpty())
    {
        QString sql = "update user_infos set role = :role where name = :name";
        m_query->prepare(sql);
        m_query->bindValue(":role", role);
        m_query->bindValue(":name", name);
    }
    else if (role.isEmpty())
    {
        QString sql = "update user_infos set password = :password where name = :name";
        m_query->prepare(sql);
        QByteArray byteArr(newpwd.toStdString().data());
        m_query->bindValue(":password", byteArr.toBase64());
        m_query->bindValue(":name", name);
    }
    else
    {
        QString sql = "update user_infos set role = :role, password = :password where name = :name";
        m_query->prepare(sql);
        m_query->bindValue(":role", role);
        QByteArray byteArr(newpwd.toStdString().data());
        m_query->bindValue(":password", byteArr.toBase64());
        m_query->bindValue(":name", name);
    }

    if (!m_query->exec())
    {
        qWarning() << "update user " << name << " failed: " << m_query->lastError().text();
        return false;
    }

    return true;
}

QString SQLConfigure::queryUser(const QString name, const QString dbpwd)
{
#ifdef SUPPOORT_SQLITECIPHER
    if (dbpwd != m_db.password())
        return "";
#endif

    if (name.isEmpty())
    {
        QString sql = "select * from user_infos";
        m_query->prepare(sql);
    }
    else
    {
        QString sql = "select * from user_infos where name = ?";
        m_query->prepare(sql);
        m_query->addBindValue(name);
    }

    if (!m_query->exec())
    {
        qWarning() << "query user infos failed: " << m_query->lastError();
        return "";
    }

    QJsonArray jsonarr;
    while (m_query->next())
    {
        QJsonObject jsonObj;
        jsonObj[PARAM_USER] = m_query->value(1).toString();
        jsonObj[PARAM_USER_ROLE] = m_query->value(2).toString();
        jsonObj[PARAM_USER_PASSWD] = QByteArray::fromBase64(m_query->value(3).toByteArray()).toStdString().data();
        jsonarr.append(jsonObj);
    }

    QJsonDocument doc(jsonarr);
    return QString::fromUtf8(doc.toJson(QJsonDocument::Compact).constData());
}
