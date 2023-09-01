#include "monitor-disk.h"
#include "ksvaudit-configure_global.h"
#include "kiran-log/qt5-log-i.h"
#include "common-definition.h"
#include <QtDBus/QDBusConnection>
#include <QtDBus/QtDBus>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <sys/vfs.h>
#include <sys/types.h>
#include <unistd.h>

extern "C" {
#include "libavformat/avformat.h"
#include "libavutil/dict.h"
}

MonitorDisk::MonitorDisk(QWidget *parent)
            : m_maxSaveDays(AUDIT_DEFAULT_CONFIG_MAX_SAVE_DAYS)
            , m_maxRecordPerUser(AUDIT_DEFAULT_CONFIG_MAX_RECORD_PER_USER)
            , m_minFreeSpace(AUDIT_DEFAULT_CONFIG_MIN_FREE_SPACE)
            , m_maxFileSize(AUDIT_DEFAULT_CONFIG_MAX_FILE_SIZE)
            , m_recordMinFreeSpace(RECORD_DEFAULT_CONFIG_MIN_FREE_SPACE)
            , m_filePath(AUDIT_DEFAULT_CONFIG_FILEPATH)
            , m_recordFilePath(RECORD_DEFAULT_CONFIG_FILEPATH)
{
    m_dbusInterface = new ConfigureInterface(KSVAUDIT_CONFIGURE_SERVICE_NAME, KSVAUDIT_CONFIGURE_PATH_NAME, QDBusConnection::systemBus(), this);
    connect(m_dbusInterface, SIGNAL(ConfigureChanged(QString, QString)), this, SLOT(UpdateConfigureData(QString, QString)));
    connect(m_dbusInterface, SIGNAL(SignalNotification(int, QString)), this, SLOT(ReceiveNotification(int, QString)));
    connect(m_dbusInterface, SIGNAL(SignalSwitchControl(int,int, QString)), this, SLOT(switchControlSlot(int, int, QString)));

    // 获取配置信息
    QString value = m_dbusInterface->GetAuditInfo();
    parseConfigureInfo(value);
    QString recordValue = m_dbusInterface->GetRecordInfo();
    parseRecordConfigureInfo(recordValue);
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
    QJsonObject jsonObj;
    if (!parseJsonData(value, jsonObj))
        return;

    for (auto key : jsonObj.keys())
    {
        // 只取需要的key，不需要考虑else
        if (GENEARL_CONFIG_FILEPATH == key)
        {
            m_filePath = jsonObj[key].toString();
            if (m_filePath.contains('~'))
            {
                KLOG_INFO() << m_filePath << "replace ~:";
                m_filePath.replace('~', getenv("HOME"));
            }
        }
        else if (GENEARL_CONFIG_MIN_FREE_SPACE == key)
        {
            m_minFreeSpace = jsonObj[key].toString().toULongLong();
        }
        else if (GENEARL_CONFIG_MAX_SAVE_DAYS == key)
        {
            m_maxSaveDays = jsonObj[key].toString().toInt();
        }
        else if (GENEARL_CONFIG_MAX_RECORD_PER_USER == key)
        {
            m_maxRecordPerUser = jsonObj[key].toString().toInt();
        }
        else if (GENEARL_CONFIG_MAX_FILE_SIZE == key)
        {
            m_maxFileSize = jsonObj[key].toString().toULongLong();
        }
        else
        {
            continue;
        }
    }

    KLOG_DEBUG() << "FilePath:" << m_filePath << "MinFreeSpace:" << m_minFreeSpace << "maxSaveDays:" << m_maxSaveDays << "MaxRecordPerUser:" << m_maxRecordPerUser;
}

//前台录屏磁盘空间检测
void MonitorDisk::checkFrontRecordFreeSpace(QString homeDir, QVector<int> pids)
{
    QString filePath = m_recordFilePath;
    if (filePath.contains('~'))
    {
        filePath.replace('~', homeDir);
    }

    //存放目录不存在时直接退出，不创建原因：以免因为权限影响前台使用目录
    if (!QFile::exists(filePath))
    {
        KLOG_WARNING() << filePath << "not exist";
        return;
    }

    struct statfs diskInfo;
    statfs(filePath.toStdString().data(), &diskInfo);
    quint64 availsize = diskInfo.f_bavail * diskInfo.f_bsize;
    if (availsize < m_recordMinFreeSpace)
    {
        KLOG_DEBUG("minFreeSpace: %lluB(%lluM)(%lluG), availsize:%lluB(%lluM)(%lluG)",
           m_recordMinFreeSpace, (m_recordMinFreeSpace>>20), (m_recordMinFreeSpace>>30), availsize, (availsize>>20), (availsize>>30));
        for (auto pid : pids)
        {
            sendSwitchControl(pid, OPERATE_DISK_FRONT_NOTIFY);
        }
    }
}

void MonitorDisk::parseRecordConfigureInfo(QString value)
{
    QJsonObject jsonObj;
    if (!parseJsonData(value, jsonObj))
        return;

    for (auto key : jsonObj.keys())
    {
        // 只取需要的key，不需要考虑else
        if (GENEARL_CONFIG_FILEPATH == key)
        {
            m_recordFilePath = jsonObj[key].toString();
        }
        else if (GENEARL_CONFIG_MIN_FREE_SPACE == key)
        {
            m_recordMinFreeSpace = jsonObj[key].toString().toULongLong();
        }
        else
        {
            continue;
        }
    }

    KLOG_INFO() << "FilePath:" << m_recordFilePath << "MinFreeSpace:" << m_recordMinFreeSpace;
}

bool MonitorDisk::parseJsonData(const QString &param,  QJsonObject &jsonObj)
{
    QString str = param;
    QJsonParseError jError;
    QJsonDocument jsonDocument = QJsonDocument::fromJson(str.toUtf8(), &jError);

    if (jsonDocument.isObject())
    {
        jsonObj = jsonDocument.object();
        return true;
    }

    //判断是否因为中文utf8导致解析失败
    KLOG_INFO() << "parse json" << jsonDocument << "err, err info:" << jError.error;
    if (QJsonParseError::ParseError::IllegalUTF8String != jError.error)
        return false;

    //QJsonDocument::fromJson解析中文utf8失败处理
    if (!param.startsWith("{") || !param.endsWith("}"))
        return false;

    QStringList jsonList = param.split(",");
    for (auto jsonString : jsonList)
    {
        QStringList dataList = jsonString.split("\"");
        int index = dataList.indexOf(":");
        if (index > 0 && index < dataList.size() - 1)
        {
            jsonObj[dataList[index-1]] = dataList[index+1];
        }
    }

    return true;
}

void MonitorDisk::fixVidoeSuffix(const QString &filePath)
{
    QDir dir(filePath);
    QStringList nameFlters;
    nameFlters << "*.mkv.tmp" << "*.mp4.tmp";
    dir.setNameFilters(nameFlters);
    dir.setFilter(QDir::Files | QDir::NoSymLinks);
    QFileInfoList videoFiles = dir.entryInfoList();
    QProcess p;
    for (auto afile : videoFiles)
    {
        // 暂时mkv没有损坏的情况，没有时长也能播放
        QString fileName(afile.absoluteFilePath());
        QString cmd = QString("lsof %1").arg(fileName);
        p.start(cmd);
        p.waitForFinished();
        if (p.readAllStandardOutput().isEmpty())
        {
            // lsof 没有标准输出则没有被占用
            // 因为monitor需要sudo权限，这里命令不需要加sudo
            KLOG_DEBUG() << "[fixVideo:fileName and baseName]: [" << fileName << "] [" << afile.completeBaseName() << "]";
            QFile tmpFile(fileName);
            QString newName = QString("%1/%2").arg(dir.absolutePath()).arg(afile.completeBaseName());
            if (fileName.endsWith(".mp4.tmp"))
            {
                if (!checkMP4Broken(fileName))
                {
                    // mp4 可能存在破损的情况，重命名成.crash结尾
                    newName = QString("%1/%2.crash").arg(dir.absolutePath()).arg(afile.completeBaseName());
                }
            }
            bool ret = tmpFile.rename(newName);
            KLOG_WARNING() << "[fixVideo:ret and newName]: [" << ret << "]" << "[" << newName << "]";
        }
    }
    p.close();
}

bool MonitorDisk::checkMP4Broken(const QString &fileAbsPath)
{
    // 检查mp4文件是否有时长，目前是没有时长的播放不了
    AVFormatContext* pCtx = nullptr;
    bool finalret = true;
    int ret = avformat_open_input(&pCtx, fileAbsPath.toStdString().c_str(), nullptr, nullptr);
    if (ret < 0)
    {
        KLOG_DEBUG() << "avformat_open_input() failed:" << ret;
        return !finalret;
    }
    int ret1 = avformat_find_stream_info(pCtx,nullptr);
    if(ret1 < 0)
    {
        KLOG_DEBUG() << "avformat_find_stream_info() failed:" << ret1;
        if (pCtx)
        {
            avformat_close_input(&pCtx);
            pCtx=nullptr;
        }
        return !finalret;
    }

    if (pCtx->duration == AV_NOPTS_VALUE)
    {
        finalret = false;
    }
    if (pCtx)
    {
        avformat_close_input(&pCtx);
        pCtx=nullptr;
    }
    return finalret;
}

void MonitorDisk::UpdateConfigureData(QString key, QString value)
{
    KLOG_INFO() << "type:" << key << "value:" << value;
    if (GENEARL_CONFIG_AUDIT == key)
    {
        parseConfigureInfo(value);
    }
    else if (GENEARL_CONFIG_RECORD == key)
    {
        parseRecordConfigureInfo(value);
    }
    else
    {
        KLOG_INFO() << "not support:" << key;
    }
}

void MonitorDisk::ReceiveNotification(int pid, QString message)
{
    KLOG_INFO() << pid << message;
    this->SignalNotification(pid, message);
}

void MonitorDisk::switchControlSlot(int from_pid, int to_pid, QString op)
{
    // 接收来自前台ui启进程的信号，并处理
    if (from_pid > 0 && to_pid == 0 && op.startsWith("process="))
    {
        KLOG_INFO() << "from_pid:" << from_pid << "to_pid:" << to_pid << "op:" << op;
        QStringList dataList = op.split(";");
        if (dataList.size() != 5 || !dataList[1].startsWith("DISPLAY=") || !dataList[2].startsWith("XAUTHORITY=")
            || !dataList[3].startsWith("USER=") || !dataList[4].startsWith("HOME="))
        {
            KLOG_INFO() << "param err";
            return;
        }

        int pidIndex = dataList[0].indexOf("=") + 1;
        QString pid = dataList[0].mid(pidIndex, dataList[0].size());
        if (pid.toInt() != from_pid)
            return;

        int nameIndex = dataList[1].indexOf("=") + 1;
        int authIndex = dataList[2].indexOf("=") + 1;
        int userIndex = dataList[3].indexOf("=") + 1;
        int homeIndex = dataList[4].indexOf("=") + 1;
        QString displayName = dataList[1].mid(nameIndex, dataList[1].size());
        QString authFile = dataList[2].mid(authIndex, dataList[2].size());
        QString userName = dataList[3].mid(userIndex, dataList[3].size());
        QString homeDir = dataList[4].mid(homeIndex, dataList[4].size());
        if (displayName.isEmpty() || userName.isEmpty() || homeDir.isEmpty())
        {
            KLOG_INFO() << "parse data err";
            return;
        }

        this->SignalFrontBackend(from_pid, displayName, authFile, userName, homeDir);
    }
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
            KLOG_INFO() << it.key() << "file reaches maximum limit" << it.value();
            // 达到最大文件大小后发送退出并保存信号，monitor收到程序退出会再拉起
            sendSwitchControl(it.key(), OPERATE_RECORD_STOP);
            sendSwitchControl(it.key(), OPERATE_RECORD_EXIT);
            map.erase(it++);
            continue;
        }

        ++it;
    }
}

void MonitorDisk::sendSwitchControl(int to_pid, const QString &operate)
{
    KLOG_INFO() << "to_pid:" << to_pid << "operate:" << operate;
    m_dbusInterface->SwitchControl(getpid(), to_pid, operate);
}

void MonitorDisk::sendProcessPid(int from_pid, int to_pid)
{
    KLOG_INFO() << "from_pid:" << from_pid << "to_pid:" << to_pid;
    m_dbusInterface->SwitchControl(from_pid, to_pid, OPERATE_FRONT_PROCESS);
}

void MonitorDisk::fixVidoes()
{
    // fixVideoSuffix() wrapper
    fixVidoeSuffix(m_recordFilePath);
    fixVidoeSuffix(m_filePath);
}

bool MonitorDisk::fileDiskLimitProcess()
{
    //存放目录不存在时创建一个
    QString filePath = m_filePath;
    if (!QFile::exists(filePath))
    {
        QDir dir;
        if (!dir.exists(filePath) && !dir.mkpath(filePath))
        {
            KLOG_WARNING() << "can't mkdir the audit file path " << filePath;
            return true;
        }
    }
    checkSaveDays(filePath, m_maxSaveDays);
    return checkFreeSpace(filePath, m_minFreeSpace);
}
