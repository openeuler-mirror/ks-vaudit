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

private:
    QMap<QString, QString> getXorgLoginName();
    QStringList getVncProcessId();
    QString getRemotIP(const QString &pid);
    QVector<sessionInfo> getXorgInfo();
    QVector<sessionInfo> getXvncInfo();
    QProcess* startRecordWithDisplay(sessionInfo info);
    void DealSession();
    bool isLicenseActive();

private slots:
    void monitorProcess();
    void receiveNotification(int, QString);

signals:
    void exitProgram();

private:
    bool m_isActive;
    QString m_filePath;
    QString m_vauditBin;
    QTimer *m_pTimer;
    QMutex m_mutex;
    QProcess *m_process;
    QMap<int, QString> m_videoFileName;
    QMultiMap<QString, sessionInfo> m_sessionInfos;
};

#endif // MONITOR_H


