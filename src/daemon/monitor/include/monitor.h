#ifndef MONITOR_H
#define MONITOR_H

#include <QObject>
#include <QProcess>
#include <QMultiMap>
#include <QTimer>
#include <QMutex>

struct sessionInfo{
    QString userName;
    QString ip;
    QString displayName;
    QString authFile;
    QProcess *process;
    bool bNotify;
    bool isRun;

    friend bool operator==(const sessionInfo &ob1, const sessionInfo &ob2)
    {
        return (ob1.userName == ob2.userName && ob1.ip == ob2.ip
                && ob1.displayName == ob2.displayName && ob1.authFile == ob2.authFile);
    }
    bool operator==(const sessionInfo &rhs)
    {
        return (userName == rhs.userName && ip == rhs.ip
                && displayName == rhs.displayName && authFile == rhs.authFile);
    }
};

class Monitor : public QObject
{
    Q_OBJECT
public:
    explicit Monitor(QObject *parent = NULL);
    ~Monitor();
    QProcess* startRecordWithDisplay(sessionInfo info);

private:
    QMap<QString, QString> getXorgLoginName();
    QStringList getVncProcessId();
    QString getRemotIP(const QString &pid);
    QVector<sessionInfo> getXorgInfo();
    QVector<sessionInfo> getXvncInfo();
    void DealSession(bool isDiskOk);
    bool isLicenseActive();
    void clearSessionInfos();
    void clearProcess(QProcess *process);
    void removeSession(sessionInfo *pInfo = nullptr, QProcess::ExitStatus = QProcess::ExitStatus::NormalExit);

private slots:
    void monitorProcess();
    void receiveNotification(int, QString);

signals:
    void exitProgram();

public:
    QMultiMap<QString, sessionInfo> m_sessionInfos;
    QMap<int, QString> m_videoFileName;

private:
    bool m_isActive;
    int m_lastMaxRecordPerUser;
    QString m_filePath;
    QString m_vauditBin;
    QTimer *m_pTimer;
    QMutex m_mutex;
    QProcess *m_process;
};

#endif // MONITOR_H


