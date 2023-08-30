#include "monitor-disk.h"
#include "ksvaudit-configure_global.h"
#include "kiran-log/qt5-log-i.h"
#include <QtDBus/QDBusConnection>
#include <QtDBus/QtDBus>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <sys/vfs.h>
#include <sys/types.h>
#include <unistd.h>

#define TIMEOUT_MS 5000

MonitorDisk::MonitorDisk(QWidget *parent) : m_maxSaveDays(30), m_maxRecordPerUser(0), m_minFreeSpace(10737418240),
                                            m_maxFileSize(2147483648), m_recordMinFreeSpace(1073741824),
                                            m_filePath("/opt/ks-vaudit"), m_recordFilePath("~/videos/ks-vaudit")
{
    m_dbusInterface = new  ConfigureInterface(KSVAUDIT_CONFIGURE_SERVICE_NAME, KSVAUDIT_CONFIGURE_PATH_NAME, QDBusConnection::systemBus(), this);
    if (!m_dbusInterface)
    {
        KLOG_WARNING() << "configure dbus Interface is null";
        exit(1);
    }
    connect(m_dbusInterface, SIGNAL(ConfigureChanged(QString, QString)), this, SLOT(UpdateConfigureData(QString, QString)));
    connect(m_dbusInterface, SIGNAL(SignalNotification(int, QString)), this, SLOT(ReceiveNotification(int, QString)));
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

    KLOG_DEBUG() << "audit: FilePath:" << m_filePath << "MinFreeSpace:" << m_minFreeSpace << "maxSaveDays:" << m_maxSaveDays << "MaxRecordPerUser:" << m_maxRecordPerUser;

    QString recordValue = m_dbusInterface->GetRecordInfo();
    parseRecordConfigureInfo(recordValue);
    KLOG_DEBUG() << "record: FilePath:" << m_recordFilePath << "MinFreeSpace" << m_recordMinFreeSpace;
}

//超出最大保存天数的录像删除
void MonitorDisk::checkSaveDays(const QString &filePath, const int &maxSaveDays)
{
    QDir dir(filePath);
    QStringList nameFilters;
    nameFilters << "*.mp4" << "*.ogv" << "*.mkv" << "*.MP4" << "*.OGV" << "*.MKV" << "*.mp4.tmp" << "*.ogv.tmp" << "*.mkv.tmp" << "*.MP4.tmp" << "*.OGV.tmp" << "*.MKV.tmp";
    QDateTime currTime = QDateTime::currentDateTime();
    QFileInfoList files = dir.entryInfoList(nameFilters, QDir::NoDotAndDotDot | QDir::Files, QDir::Time | QDir::Reversed); //Sort by time (modification time)
    for (auto file : files)
    {
        const qint64 &diffTime = file.lastModified().daysTo(currTime);
        if (diffTime > maxSaveDays)
        {
            KLOG_WARNING() << "remove file:" << file.absoluteFilePath() << "filetime:" << file.lastModified().date() << "currtime:" << currTime.date() << "diff days:" << diffTime ;
            dir.remove(file.absoluteFilePath());
        }
    }
}

bool MonitorDisk::checkFreeSpace(const QString &filePath, const quint64 &minFreeSpace)
{
    QDir dir(filePath);
    struct statfs diskInfo;
    statfs(filePath.toStdString().data(), &diskInfo);
    quint64 availsize = diskInfo.f_bavail * diskInfo.f_bsize;

    //少于最小可用空间，停止录像并弹框提示
    if (availsize < minFreeSpace)
    {
        //KLOG_DEBUG() << "availsize" << availsize << "minFreeSpace" << minFreeSpace << "filePath:" << filePath;
        return false;
    }

    return true;
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
        else if ("MaxFileSize" == key)
        {
            m_maxFileSize = jsonObj[key].toString().toULongLong();
        }
    }
}

void MonitorDisk::checkRecordFreeSpace(QString filePath, const quint64 &minFreeSpace)
{
    if (!filePathOk(filePath))
        return;

    struct statfs diskInfo;
    statfs(filePath.toStdString().data(), &diskInfo);
    quint64 availsize = diskInfo.f_bavail * diskInfo.f_bsize;
    if (availsize < minFreeSpace)
    {
        //KLOG_DEBUG("minFreeSpace: %lluB(%lluM)(%lluG), availsize:%lluB(%lluM)(%lluG)",
        //    minFreeSpace, (minFreeSpace>>20), (minFreeSpace>>30), availsize, (availsize>>20), (availsize>>30));
        sendSwitchControl(0, "DiskSpace");
    }
}

void MonitorDisk::parseRecordConfigureInfo(QString value)
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
            m_recordFilePath = jsonObj[key].toString();
        }
        else if ("MinFreeSpace" == key)
        {
            m_recordMinFreeSpace = jsonObj[key].toString().toULongLong();
        }
    }
}

bool MonitorDisk::filePathOk(QString &filePath)
{
    if (filePath.isEmpty())
        return false;

    if (filePath.contains('~'))
        filePath.replace('~', getenv("HOME"));

    QDir dir(filePath);
    if (!dir.exists())
        return false;

    return true;
}

void MonitorDisk::UpdateConfigureData(QString key, QString value)
{
    if ("audit" == key)
    {
        parseConfigureInfo(value);
        KLOG_DEBUG() << "audit: FilePath:" << m_filePath << "MinFreeSpace:" << m_minFreeSpace << "maxSaveDays:" << m_maxSaveDays << "MaxRecordPerUser:" << m_maxRecordPerUser << "m_maxFileSize:" << m_maxFileSize;
    }
    else if ("record" == key)
    {
        parseRecordConfigureInfo(value);
        KLOG_DEBUG() << "record: FilePath:" << m_recordFilePath << "MinFreeSpace" << m_recordMinFreeSpace;
    }
}

void MonitorDisk::ReceiveNotification(int pid, QString message)
{
    KLOG_DEBUG() << pid << message;
    this->SignalNotification(pid, message);
}

int MonitorDisk::getMaxRecordPerUser()
{
    return m_maxRecordPerUser;
}

void MonitorDisk::fileSizeProcess(QMap<int, QString>& map)
{
    for (auto it = map.begin(); it != map.end();)
    {
        QFileInfo fileinfo(it.value());
        if (fileinfo.size() >= m_maxFileSize)
        {
            KLOG_DEBUG() << "file reaches maximum limit" << it.value();
            sendSwitchControl(it.key(), "stop");
            sendSwitchControl(it.key(), "start");
            map.erase(it++);
            continue;
        }

        ++it;
    }
}

void MonitorDisk::sendSwitchControl(int to_pid, const QString &operate)
{
    KLOG_DEBUG() << "to_pid:" << to_pid << "operate:" << operate;
    m_dbusInterface->SwitchControl(getpid(), to_pid, operate);
}

bool MonitorDisk::fileDiskLimitProcess()
{
    checkRecordFreeSpace(m_recordFilePath, m_recordMinFreeSpace); //前台录屏磁盘空间检测

    QString filePath = m_filePath;
    if (!filePathOk(filePath))
        return true;

    int maxSaveDays = m_maxSaveDays;
    checkSaveDays(filePath, maxSaveDays);

    quint64 minFreeSpace = m_minFreeSpace;
    return checkFreeSpace(filePath, minFreeSpace);
}
