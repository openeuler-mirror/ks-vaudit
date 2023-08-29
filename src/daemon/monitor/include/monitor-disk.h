#ifndef MONITOR_DISK_H
#define MONITOR_DISK_H

#include "configure_interface.h"

#include <QObject>
#include <QString>

class MonitorDisk : public QObject
{
    Q_OBJECT
public:
    MonitorDisk(QWidget *parent = 0);
    static MonitorDisk &instance();
    void fileDiskLimitProcess();
    int getMaxRecordPerUser();

private:
    ~MonitorDisk();
    void getAuditInfo();
    void checkSaveDays(const QString &filePath, const int &maxSaveDays);
    void checkFreeSpace(const QString &filePath, const quint64 &minFreeSpace);
    void parseConfigureInfo(QString);

private slots:
    void UpdateConfigureData(QString, QString);

private:
    int m_maxSaveDays;
    int m_maxRecordPerUser;
    quint64 m_minFreeSpace;
    QString m_filePath;
    ConfigureInterface *m_dbusInterface;
};

#endif
