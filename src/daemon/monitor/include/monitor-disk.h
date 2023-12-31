#ifndef MONITOR_DISK_H
#define MONITOR_DISK_H

#include "configure_interface.h"

#include <QObject>
#include <QString>
#include <QMap>

class MonitorDisk : public QObject
{
    Q_OBJECT
public:
    MonitorDisk(QWidget *parent = 0);
    static MonitorDisk &instance();
    bool fileDiskLimitProcess();
    int getMaxRecordPerUser();
    void fileSizeProcess(QMap<int, QString>&);
    void sendSwitchControl(int to_pid, const QString &operate);
    void sendProcessPid(int from_pid, int to_pid);
    void fixVidoes();
    void checkFrontRecordFreeSpace(QString, QVector<int>);

private:
    ~MonitorDisk();
    void checkSaveDays(const QString &filePath, const int &maxSaveDays);
    bool checkFreeSpace(const QString &filePath, const quint64 &minFreeSpace);
    void parseConfigureInfo(QString);
    void parseRecordConfigureInfo(QString value);
    bool filePathOk(QString &filePath);
    bool parseJsonData(const QString &param,  QJsonObject &jsonObj);
    void fixVidoeSuffix(const QString &filePath);
    bool checkMP4Broken(const QString &fileAbsPath);

private slots:
    void UpdateConfigureData(QString, QString);
    void ReceiveNotification(int, QString);
    void switchControlSlot(int, int, QString);

signals:
    void SignalNotification(int pid, const QString &message);
    void SignalFrontBackend(int, QString, QString, QString, QString);

private:
    int m_maxSaveDays;
    int m_maxRecordPerUser;
    quint64 m_minFreeSpace;
    quint64 m_maxFileSize;
    quint64 m_recordMinFreeSpace;
    QString m_filePath;
    QString m_recordFilePath;
    ConfigureInterface *m_dbusInterface;
};

#endif
