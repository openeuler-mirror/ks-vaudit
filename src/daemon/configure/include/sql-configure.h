#ifndef SQLCONFIGURE_H
#define SQLCONFIGURE_H

#include <QSqlDatabase>
#include <QMutex>

class SQLConfigure
{
public:
    static SQLConfigure &Instance();
    bool createUser(const QString);
    bool deleteUser(const QString);
    bool updateUser(const QString);
    QString queryUser(const QString);

private:
    SQLConfigure();
    ~SQLConfigure();
    bool initDB();
    bool checkUserPasswd(const QString name, const QString pwd);
    bool isNameLegal(QString);
    bool createUser(const QString name, const QString pwd, const QString role);
    bool deleteUser(const QString name, const QString pwd);
    bool updateUser(const QString name, const QString oldpwd, const QString newpwd, const QString role);
    QString queryUser(const QString name, const QString dbpwd);

private:
    QString m_usr;
    QString m_pwd;
    QSqlDatabase m_db;
    QSqlQuery *m_query;
    QMutex m_mutex;
};



#endif // SQLCONFIGURE_H
