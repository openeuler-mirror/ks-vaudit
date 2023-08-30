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
    bool bNotify;   // 磁盘空间不足是否在提示
    bool bStart;    // 是否开始录屏
    time_t stTime;  // 启动进程的时间，仅用于录屏程序被杀掉后判断查找到的是否为同一进程

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


