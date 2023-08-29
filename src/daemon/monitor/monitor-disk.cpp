#include "monitor-disk.h"
#include "ksvaudit-configure_global.h"
#include "kiran-log/qt5-log-i.h"
#include <QtDBus/QDBusConnection>
#include <QtDBus/QtDBus>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <sys/vfs.h>

#define TIMEOUT_MS 5000

MonitorDisk::MonitorDisk(QWidget *parent)
{
    m_dbusInterface = new  ConfigureInterface(KSVAUDIT_CONFIGURE_SERVICE_NAME, KSVAUDIT_CONFIGURE_PATH_NAME, QDBusConnection::systemBus(), this);
    if (!m_dbusInterface)
    {
        KLOG_WARNING() << "configure dbus Interface is null";
        exit(1);
    }
    connect(m_dbusInterface, SIGNAL(ConfigureChanged(QString, QString)), this, SLOT(UpdateConfigureData(QString, QString)));
    getAuditInfo();
}

MonitorDisk &MonitorDisk::instance()
{
    static MonitorDisk s_disk;
    return s_disk;
}

MonitorDisk::~MonitorDisk()
{
    if (m_dbusInterface)
        delete m_dbusInterface;
    m_dbusInterface = nullptr;
}

void MonitorDisk::getAuditInfo()
{
    QString value = m_dbusInterface->GetAuditInfo();
    parseConfigureInfo(value);

    KLOG_DEBUG() << "FilePath:" << m_filePath << "MinFreeSpace:" << m_minFreeSpace << "maxSaveDays:" << m_maxSaveDays << "MaxRecordPerUser:" << m_maxRecordPerUser;
}

void MonitorDisk::checkSaveDays(const QString &filePath, const int &maxSaveDays)
{
    QDir dir(filePath);
    QDateTime currTime = QDateTime::currentDateTime();
    QFileInfoList files = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::Files, QDir::Time | QDir::Reversed); //Sort by time (modification time)
    for (auto file : files)
    {
        QString suffix = file.suffix();
        if (suffix == "mp4" || suffix == "MP4" || suffix == "ogv" || suffix == "OGV")
        {
            const qint64 &diffTime = file.lastModified().daysTo(currTime);
            if (diffTime > maxSaveDays)
            {
                KLOG_DEBUG() << "remove file:" << file.absoluteFilePath() << "filetime:" << file.lastModified().date() << "currtime:" << currTime.date() << "diff days:" << diffTime ;
                dir.remove(file.absoluteFilePath());
            }
        }
    }
}

void MonitorDisk::checkFreeSpace(const QString &filePath, const quint64 &minFreeSpace)
{
    QDir dir(filePath);
    struct statfs diskInfo;
    statfs(filePath.toStdString().data(), &diskInfo);
    quint64 availsize = diskInfo.f_bavail * diskInfo.f_bsize;
    KLOG_DEBUG("minFreeSpace: %lluB(%lluM)(%lluG), availsize:%lluB(%lluM)(%lluG)",
               minFreeSpace, (minFreeSpace>>20), (minFreeSpace>>30), availsize, (availsize>>20), (availsize>>30));
    //少于最小可用空间，删除文件，文件格式：张三_192.168.1.1_20220621_153620.mp4
    if (availsize < minFreeSpace)
    {
        qint64 removeSize = minFreeSpace - availsize;
        KLOG_INFO("need remove: %lluB(%lluM)", removeSize, (removeSize>>20));
        QFileInfoList files = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::Files, QDir::Time | QDir::Reversed); //Sort by time (modification time)
        for (auto file : files)
        {
            QString suffix = file.suffix();
            if (suffix == "mp4" || suffix == "MP4" || suffix == "ogv" || suffix == "OGV")
            {
                removeSize -= file.size();
                KLOG_DEBUG() << "remove file:" << file.absoluteFilePath() << "time:" << file.lastModified().toLocalTime().toString() << "size:" << file.size();
                dir.remove(file.absoluteFilePath());
                if (removeSize <= 0)
                {
                    KLOG_DEBUG("finish clearing files, removeSize: %lldB(%lldM)", removeSize, (removeSize>>20));
                    return;
                }
            }
        }
    }
}

void MonitorDisk::parseConfigureInfo(QString value)
{
    QJsonDocument doc = QJsonDocument::fromJson(value.toUtf8());
    if (!doc.isObject())
    {
        KLOG_INFO() << "Cann't get the DBus configure!";
        return;
    }

    QJsonObject jsonObj = doc.object();
    for (auto key : jsonObj.keys())
    {
        if ("FilePath" == key)
        {
            m_filePath = jsonObj[key].toString();
        }
        else if ("MinFreeSpace" == key)
        {
            m_minFreeSpace = jsonObj[key].toString().toULongLong();
        }
        else if ("MaxSaveDays" == key)
        {
            m_maxSaveDays = jsonObj[key].toString().toInt();
        }
        else if ("MaxRecordPerUser" == key)
        {
            m_maxRecordPerUser = jsonObj[key].toString().toInt();
        }
    }
}

void MonitorDisk::UpdateConfigureData(QString key, QString value)
{
    if ("audit" == key)
    {
        parseConfigureInfo(value);
        KLOG_DEBUG() << "FilePath:" << m_filePath << "MinFreeSpace:" << m_minFreeSpace << "maxSaveDays:" << m_maxSaveDays << "MaxRecordPerUser:" << m_maxRecordPerUser;
    }
}

int MonitorDisk::getMaxRecordPerUser()
{
    return m_maxRecordPerUser;
}

void MonitorDisk::fileDiskLimitProcess()
{
    QString filePath = m_filePath;
    if (filePath.contains('~'))
    {
        filePath.replace('~', getenv("HOME"));
        KLOG_DEBUG() << "filePath:" << filePath;
    }

    QDir dir(filePath);
    if (!dir.exists())
    {
        KLOG_DEBUG() << "filePath:" << filePath << "not exist";
        return;
    }

    int maxSaveDays = m_maxSaveDays;
    checkSaveDays(filePath, maxSaveDays);

    quint64 minFreeSpace = m_minFreeSpace;
    checkFreeSpace(filePath, minFreeSpace);
}
