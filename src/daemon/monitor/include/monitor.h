#ifndef MONITOR_H
#define MONITOR_H

#include <QObject>
#include <QProcess>
#include <QMultiMap>
#include <QTimer>
#include <QMutex>

// 后台审计录屏专用
struct sessionInfo {
    QString userName;
    QString ip;     // 本地为127.0.0.1, 远程根据netstat获取
    QString displayName;
    QString authFile;
    QProcess *process;
    bool bNotify;   // 磁盘空间不足是否在提示
    bool bStart;    // 是否开始录屏
    time_t stTime;  // 启动进程的时间，仅用于录屏程序被杀掉后判断查找到的是否为同一进程
    bool bLocal;    // 用于判断是否本地会话，本地会话只有激活用户才启进程
    bool bActive;   // 用于判断录屏进程是否被QApplication卡住

    // 只判断固定不会变的参数
    friend bool operator==(const sessionInfo &ob1, const sessionInfo &ob2)
    {
        return (ob1.userName == ob2.userName && ob1.ip == ob2.ip
                && ob1.displayName == ob2.displayName && ob1.authFile == ob2.authFile && ob1.bLocal == ob2.bLocal);
    }
    bool operator==(const sessionInfo &rhs)
    {
        return (userName == rhs.userName && ip == rhs.ip
                && displayName == rhs.displayName && authFile == rhs.authFile && bLocal == rhs.bLocal);
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
    void clearFrontRecordInfos();
    QString frontGetXAuth(QString userName, QString display);
    bool processExist(int pid);
    QString getCurrentSessionDisplay();
    bool creatLicenseObjectName();

private slots:
    void monitorProcess();
    void recordProcess(); //前台监控
    void receiveNotification(int, QString);
    void receiveFrontBackend(int, QString, QString, QString, QString);

signals:
    void exitProgram();

private:
    bool m_isActive;
    int m_lastMaxRecordPerUser;
    QString m_filePath;
    QString m_vauditBin;
    QTimer *m_pTimer;
    QProcess *m_process;
    //后台审计会话信息，key:用户名
    QMultiMap<QString, sessionInfo> m_sessionInfos;
    QMap<int, QString> m_videoFileName;
    //前台pid对应的后台进程，前台非法关闭
    QMap<int, QProcess *> m_frontRecordInfo;
    QTimer *m_pFontTimer;
    // 存储前台录屏目录信息，key: 用户目录，value: 对应目录为key的进程pid
    QMap<QString, QVector<int>> m_frontHomeInfo;
};

#endif // MONITOR_H


