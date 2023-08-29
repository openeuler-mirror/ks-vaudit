#include "general-configure.h"
#include "kiran-log/qt5-log-i.h"
#include <QDir>
#include <QFileInfo>
#include <QJsonObject>
#include <QJsonDocument>
#include <QMutexLocker>
#include <QTextCodec>

//conatin MIN, contain MAX
#define FPS_MIN_VALUE               2
#define FPS_MAX_VALUE               60
#define BITRATE_MIN_VALUE           1
#define BITRATE_MAX_VALUE           256
#define VOLUME_MAX_VALUE            100
#define WATER_PRINT_MAX_SIZE        64
#define CONFIG_TIME_MINUTE          60
#define FREE_SPACE_MIN_VALUE        (5*1024*1024*1024ULL)
#define CONFIG_INT_MAX              (INT_MAX)
#define CONFIG_ULL_MAX              (ULLONG_MAX)
#define FILE_MIN_SIZE               (1024*1024ULL)
#define FILE_MAX_SIZE               (16*1024*1024*1024ULL)

GeneralConfigure::GeneralConfigure(QObject *parent)
{
    m_confFile = "/etc/ks-vaudit/ks-vaudit.conf";
    m_itemMap.insert(CONFIG_RECORD, "record");
    m_itemMap.insert(CONFIG_RECORD_FILEPATH, "record/FilePath");
    m_itemMap.insert(CONFIG_RECORD_FILETYPE, "record/FileType");
    m_itemMap.insert(CONFIG_RECORD_RECORD_VIDIO, "record/RecordVideo");
    m_itemMap.insert(CONFIG_RECORD_FPS, "record/Fps");
    m_itemMap.insert(CONFIG_RECORD_QUALITY, "record/Quality");
    m_itemMap.insert(CONFIG_RECORD_RECORD_AUDIO, "record/RecordAudio");
    m_itemMap.insert(CONFIG_RECORD_BITRATE, "record/Bitrate");
    m_itemMap.insert(CONFIG_RECORD_WATER_PRINT, "record/WaterPrint");
    m_itemMap.insert(CONFIG_RECORD_WATER_PRINT_TEXT, "record/WaterPrintText");
    m_itemMap.insert(CONFIG_RECORD_TIMING_REMINDER, "record/TimingReminder");
    m_itemMap.insert(CONFIG_RECORD_MIC_VOLUME, "record/MicVolume");
    m_itemMap.insert(CONFIG_RECORD_SPEAKER_VOLUME, "record/SpeakerVolume");
    m_itemMap.insert(CONFIG_RECORD_MIN_FREE_SPACE, "record/MinFreeSpace");
    m_itemMap.insert(CONFIG_AUDIT, "audit");
    m_itemMap.insert(CONFIG_AUDIT_FILEPATH, "audit/FilePath");
    m_itemMap.insert(CONFIG_AUDIT_FILETYPE, "audit/FileType");
    m_itemMap.insert(CONFIG_AUDIT_RECORD_VIDIO, "audit/RecordVideo");
    m_itemMap.insert(CONFIG_AUDIT_FPS, "audit/Fps");
    m_itemMap.insert(CONFIG_AUDIT_QUALITY, "audit/Quality");
    m_itemMap.insert(CONFIG_AUDIT_RECORD_AUDIO, "audit/RecordAudio");
    m_itemMap.insert(CONFIG_AUDIT_BITRATE, "audit/Bitrate");
    m_itemMap.insert(CONFIG_AUDIT_TIMING_PAUSE, "audit/TimingPause");
    m_itemMap.insert(CONFIG_AUDIT_WATER_PRINT, "audit/WaterPrint");
    m_itemMap.insert(CONFIG_AUDIT_WATER_PRINT_TEXT, "audit/WaterPrintText");
    m_itemMap.insert(CONFIG_AUDIT_MIN_FREE_SPACE, "audit/MinFreeSpace");
    m_itemMap.insert(CONFIG_AUDIT_MAX_SAVE_DAYS, "audit/MaxSaveDays");
    m_itemMap.insert(CONFIG_AUDIT_MAX_FILE_SIZE, "audit/MaxFileSize");
    m_itemMap.insert(CONFIG_AUDIT_MAX_RECORD_PER_USER, "audit/MaxRecordPerUser");

    m_confSettings = QSharedPointer<QSettings>(new QSettings(m_confFile, QSettings::IniFormat));
    m_confSettings->setIniCodec(QTextCodec::codecForName("UTF-8"));
    initConfig();

    m_fileWatcher = new QFileSystemWatcher();
    connect(m_fileWatcher, SIGNAL(directoryChanged(QString)), this, SLOT(onDirectoryChanged(QString)));
    m_fileWatcher->addPath("/etc/ks-vaudit/");
}

GeneralConfigure &GeneralConfigure::Instance()
{
    static GeneralConfigure s_general;
    return s_general;
}

bool GeneralConfigure::setRecordConfigure(const QString &param)
{
    QString str = param;
    QJsonDocument jsonDocument = QJsonDocument::fromJson(str.toUtf8());
    if (!jsonDocument.isObject())
        return false;

    QJsonObject jsonObj = jsonDocument.object();
    for (auto k : jsonObj.keys())
    {
        if (!checkRecordParam(fullKey(m_itemMap[CONFIG_RECORD], k), jsonObj[k].toString()))
            return false;
    }

    m_confSettings->beginGroup(m_itemMap[CONFIG_RECORD]);
    for (auto k : jsonObj.keys())
    {
        m_confSettings->setValue(k, jsonObj[k].toString());
        QString key = fullKey(m_itemMap[CONFIG_RECORD], k);
        m_lastMap[key] = jsonObj[k].toString();

        KLOG_DEBUG() << "write " << k << jsonObj[k].toString() << key << m_lastMap[key];
    }

    m_confSettings->endGroup();

    return true;
}

bool GeneralConfigure::setAuditConfigure(const QString &param)
{
    QString str = param;
    QJsonDocument jsonDocument = QJsonDocument::fromJson(str.toUtf8());
    if (!jsonDocument.isObject())
        return false;

    QJsonObject jsonObj = jsonDocument.object();
    for (auto k : jsonObj.keys())
    {
        if (!checkAuditParam(fullKey(m_itemMap[CONFIG_AUDIT], k), jsonObj[k].toString()))
            return false;
    }

    m_confSettings->beginGroup(m_itemMap[CONFIG_AUDIT]);
    for (auto k : jsonObj.keys())
    {
        m_confSettings->setValue(k, jsonObj[k].toString());
        QString key = fullKey(m_itemMap[CONFIG_AUDIT], k);
        m_lastMap[key] = jsonObj[k].toString();
        KLOG_DEBUG() << "write " << k << jsonObj[k].toString() << key << m_lastMap[key];
    }

    m_confSettings->endGroup();

    return true;
}

QString GeneralConfigure::readRecordConf()
{
    return readGroupConfig(m_itemMap[CONFIG_RECORD]);
}

QString GeneralConfigure::readAuditConf()
{
    return readGroupConfig(m_itemMap[CONFIG_AUDIT]);
}

GeneralConfigure::~GeneralConfigure()
{
    if (m_fileWatcher)
        delete m_fileWatcher;
    m_fileWatcher = nullptr;
}

void GeneralConfigure::initConfig()
{
    QFileInfo fileinfo(m_confSettings->fileName());
    KLOG_INFO() << m_confSettings->fileName() << fileinfo.isFile();

    if (fileinfo.isFile() && fileinfo.size())
    {
        m_confSettings->sync();
        bool bAdd = m_itemMap.size() > m_confSettings->allKeys().size() + 2;
        if (bAdd)
        {
            initData();
        }

        for (auto key : m_confSettings->allKeys())
        {
            m_lastMap.insert(key, m_confSettings->value(key).toString());
        }

        if (bAdd)
        {
            for (auto it = m_lastMap.begin(); it != m_lastMap.end(); it++)
            {
                m_confSettings->setValue(it.key(), it.value());
            }
            m_confSettings->sync();
        }
    }
    else
    {
        initData();
        for (auto it = m_lastMap.begin(); it != m_lastMap.end(); it++)
        {
            m_confSettings->setValue(it.key(), it.value());
        }
        m_confSettings->sync();
    }
}

void GeneralConfigure::initData()
{
    m_lastMap.insert(m_itemMap[CONFIG_RECORD_FILEPATH], "~/videos/ks-vaudit");
    m_lastMap.insert(m_itemMap[CONFIG_RECORD_FILETYPE], "MP4");
    m_lastMap.insert(m_itemMap[CONFIG_RECORD_RECORD_VIDIO], "1");
    m_lastMap.insert(m_itemMap[CONFIG_RECORD_FPS], "25");
    m_lastMap.insert(m_itemMap[CONFIG_RECORD_QUALITY], "1");
    m_lastMap.insert(m_itemMap[CONFIG_RECORD_RECORD_AUDIO], "all");
    m_lastMap.insert(m_itemMap[CONFIG_RECORD_BITRATE], "128");
    m_lastMap.insert(m_itemMap[CONFIG_RECORD_WATER_PRINT], "0");
    m_lastMap.insert(m_itemMap[CONFIG_RECORD_WATER_PRINT_TEXT], "");
    m_lastMap.insert(m_itemMap[CONFIG_RECORD_TIMING_REMINDER], "30");
    m_lastMap.insert(m_itemMap[CONFIG_RECORD_MIC_VOLUME], "50");
    m_lastMap.insert(m_itemMap[CONFIG_RECORD_SPEAKER_VOLUME], "50");
    m_lastMap.insert(m_itemMap[CONFIG_RECORD_MIN_FREE_SPACE], "1073741824");

    m_lastMap.insert(m_itemMap[CONFIG_AUDIT_FILEPATH], "/opt/ks-vaudit");
    m_lastMap.insert(m_itemMap[CONFIG_AUDIT_FILETYPE], "MP4");
    m_lastMap.insert(m_itemMap[CONFIG_AUDIT_RECORD_VIDIO], "1");
    m_lastMap.insert(m_itemMap[CONFIG_AUDIT_FPS], "10");
    m_lastMap.insert(m_itemMap[CONFIG_AUDIT_QUALITY], "1");
    m_lastMap.insert(m_itemMap[CONFIG_AUDIT_RECORD_AUDIO], "none");
    m_lastMap.insert(m_itemMap[CONFIG_AUDIT_BITRATE], "128");
    m_lastMap.insert(m_itemMap[CONFIG_AUDIT_WATER_PRINT], "1");
    m_lastMap.insert(m_itemMap[CONFIG_AUDIT_WATER_PRINT_TEXT], "");
    m_lastMap.insert(m_itemMap[CONFIG_AUDIT_TIMING_PAUSE], "5");
    m_lastMap.insert(m_itemMap[CONFIG_AUDIT_MIN_FREE_SPACE], "10737418240");
    m_lastMap.insert(m_itemMap[CONFIG_AUDIT_MAX_SAVE_DAYS], "30");
    m_lastMap.insert(m_itemMap[CONFIG_AUDIT_MAX_FILE_SIZE], "2147483648");
    m_lastMap.insert(m_itemMap[CONFIG_AUDIT_MAX_RECORD_PER_USER], "0");
}

QString GeneralConfigure::readGroupConfig(QString group)
{
    QJsonObject jsonObj;
    m_confSettings->beginGroup(group);
    auto keys = m_confSettings->childKeys();
    for (auto key : keys)
    {
        jsonObj[key] = m_confSettings->value(key).toString();
    }
    m_confSettings->endGroup();

    QJsonDocument doc(jsonObj);
    return QString::fromUtf8(doc.toJson(QJsonDocument::Compact).constData());
}

QString GeneralConfigure::fullKey(QString group, QString key)
{
    return group + "/" + key;
}

bool GeneralConfigure::checkRecordParam(QString item, QString value)
{
    if (m_itemMap[CONFIG_RECORD_FILEPATH] == item)
        return checkPath(value);
    if (m_itemMap[CONFIG_RECORD_FILETYPE] == item)
        return checkType(value);
    if (m_itemMap[CONFIG_RECORD_RECORD_VIDIO] == item)
        return checkEnable(value);
    if (m_itemMap[CONFIG_RECORD_FPS] == item)
        return checkInteger(value, FPS_MIN_VALUE, FPS_MAX_VALUE);
    if (m_itemMap[CONFIG_RECORD_QUALITY] == item)
        return checkInteger(value, 0, 2);
    if (m_itemMap[CONFIG_RECORD_RECORD_AUDIO] == item)
        return checkRecordAudio(value);
    if (m_itemMap[CONFIG_RECORD_BITRATE] == item)
        return checkInteger(value, BITRATE_MIN_VALUE, BITRATE_MAX_VALUE);
    if (m_itemMap[CONFIG_RECORD_WATER_PRINT] == item)
        return checkEnable(value);
    if (m_itemMap[CONFIG_RECORD_WATER_PRINT_TEXT] == item)
        return checkWaterPrint(value);
    if (m_itemMap[CONFIG_RECORD_TIMING_REMINDER] == item)
        return checkInteger(value, 0, CONFIG_TIME_MINUTE);
    if (m_itemMap[CONFIG_RECORD_MIC_VOLUME] == item)
        return checkInteger(value, 0, VOLUME_MAX_VALUE);
    if (m_itemMap[CONFIG_RECORD_SPEAKER_VOLUME] == item)
        return checkInteger(value, 0, VOLUME_MAX_VALUE);
    if (m_itemMap[CONFIG_RECORD_MIN_FREE_SPACE] == item)
        return checkULongLong(value, 0, CONFIG_ULL_MAX);

    KLOG_INFO() << item << " is not in the record configure options";
    return false;
}

bool GeneralConfigure::checkAuditParam(QString item, QString value)
{
    if (m_itemMap[CONFIG_AUDIT_FILEPATH] == item)
        return checkPath(value);
    if (m_itemMap[CONFIG_AUDIT_FILETYPE] == item)
        return checkType(value);
    if (m_itemMap[CONFIG_AUDIT_RECORD_VIDIO] == item)
        return checkEnable(value);
    if (m_itemMap[CONFIG_AUDIT_FPS] == item)
        return checkInteger(value, FPS_MIN_VALUE, FPS_MAX_VALUE);
    if (m_itemMap[CONFIG_AUDIT_QUALITY] == item)
        return checkInteger(value, 0, 2);
    if (m_itemMap[CONFIG_AUDIT_RECORD_AUDIO] == item)
        return checkRecordAudio(value);
    if (m_itemMap[CONFIG_AUDIT_BITRATE] == item)
        return checkInteger(value, BITRATE_MIN_VALUE, BITRATE_MAX_VALUE);
    if (m_itemMap[CONFIG_AUDIT_WATER_PRINT] == item)
        return checkEnable(value);
    if (m_itemMap[CONFIG_AUDIT_WATER_PRINT_TEXT] == item)
        return checkWaterPrint(value);
    if (m_itemMap[CONFIG_AUDIT_TIMING_PAUSE] == item)
        return checkInteger(value, 0, CONFIG_TIME_MINUTE);
    if (m_itemMap[CONFIG_AUDIT_MIN_FREE_SPACE] == item)
        return checkULongLong(value, FREE_SPACE_MIN_VALUE, CONFIG_ULL_MAX);
    if (m_itemMap[CONFIG_AUDIT_MAX_SAVE_DAYS] == item)
        return checkInteger(value, 0, CONFIG_INT_MAX);
    if (m_itemMap[CONFIG_AUDIT_MAX_FILE_SIZE] == item)
        return checkULongLong(value, FILE_MIN_SIZE, FILE_MAX_SIZE);
    if (m_itemMap[CONFIG_AUDIT_MAX_RECORD_PER_USER] == item)
        return checkInteger(value, 0, CONFIG_INT_MAX);

    KLOG_INFO() << item << " is not in the audit configure options";
    return false;
}

bool GeneralConfigure::checkPath(QString path)
{
    if (path.contains('~'))
    {
        QString home_dir = getenv("HOME");
        path.replace('~', home_dir);
        KLOG_INFO() << "repalce path:" << path;
    }

    QDir dir(path);
    if (dir.exists())
        return true;

    KLOG_INFO() << "path: " << path << " is not exist";
    return false;
}

bool GeneralConfigure::checkType(QString type)
{
    if (type == "mp4" || type == "MP4" || type == "ogv" || type == "OGV")
        return true;

    KLOG_INFO() << "file type is " << type << ", and needs to be mp4 or ogv";
    return false;
}

bool GeneralConfigure::checkEnable(QString enable)
{
    bool ok{};
    int tmp = enable.toInt(&ok);
    if (!ok || (tmp != 0 && tmp != 1))
    {
        KLOG_INFO() << enable << " is err, needs to be 0 or 1";
        return false;
    }

    return true;
}

bool GeneralConfigure::checkInteger(QString value, int min, int max)
{
    bool ok{};
    int tmp = value.toInt(&ok);
    if (!ok || tmp < min || tmp > max)
    {
        KLOG_INFO() << value << " is err, needs to be " << min << " - " << max;
        return false;
    }

    return true;
}

bool GeneralConfigure::checkULongLong(QString value, unsigned long long min, unsigned long long max)
{
    bool ok{};
    unsigned long long tmp = value.toULongLong(&ok);
    if (!ok || tmp < min || tmp > max)
    {
        KLOG_INFO() << value << " is err, needs to be " << min << " - " << max;
        return false;
    }

    return true;
}

bool GeneralConfigure::checkRecordAudio(QString value)
{
    if (value == "all" || value == "none" || value == "mic" || value == "speaker")
        return true;

    KLOG_INFO() << "record audio is " << value << ", and needs to be all or none or mic or speaker";
    return false;
}

bool GeneralConfigure::checkWaterPrint(QString value)
{
    if (value.size() > WATER_PRINT_MAX_SIZE)
    {
        KLOG_INFO() << "record water print too big";
        return false;
    }

    return true;
}

void GeneralConfigure::rewriteConfig()
{
    QFile file(m_confFile);
    file.open(QFile::WriteOnly | QFile::Truncate); //清空文件
    file.close();

    for (auto it = m_lastMap.begin(); it != m_lastMap.end(); it++)
    {
        m_confSettings->setValue(it.key(), it.value());
    }
    m_confSettings->sync();
}

void GeneralConfigure::onDirectoryChanged(QString)
{
    QMutexLocker locker(&m_mutex);
    int fileSize{};
    m_confSettings->sync();
    QFile file(m_confFile);
    if (!file.open(QIODevice::ReadOnly))
    {
        KLOG_INFO() <<  "fail to open the file: " << m_confFile;
        return;
    }
    fileSize = file.size();
    file.close();

    if (fileSize && m_confSettings->allKeys().size() == 0)
        return;

    QMap<QString, QString> dataMap;
    for (auto key : m_confSettings->allKeys())
    {
        dataMap.insert(key,  m_confSettings->value(key).toString());
    }

    if (dataMap.size() != m_lastMap.size())
    {
        KLOG_INFO() << "number of options changed externally, restore file, old:" << m_lastMap.size() << "new:" << dataMap.size() << "fileSize:" << fileSize;
        rewriteConfig();
        return;
    }

    if (dataMap == m_lastMap)
        return;

    if (dataMap.keys() != m_lastMap.keys())
    {
        KLOG_INFO() << "options changed, restore file, old:" << m_lastMap.keys() << "new:" << dataMap.keys();
        rewriteConfig();
        return;
    }
 
    KLOG_INFO() << "data changed";
    QJsonObject recordObj, auditObj;
    for (auto k : dataMap.keys())
    {
        if (m_lastMap[k] != dataMap[k])
        {
            auto key = k.split("/");
            KLOG_INFO() << k << key[0] << key[1] << m_lastMap[k] << dataMap[k];
            bool ret = (key[0] == m_itemMap[CONFIG_RECORD] ? checkRecordParam(k, dataMap[k]) : checkAuditParam(k, dataMap[k]));
            if (ret)
            {
                if (key[0] == m_itemMap[CONFIG_RECORD])
                    recordObj[key[1]] = dataMap[k];
                else
                    auditObj[key[1]] = dataMap[k];
                m_lastMap[k] = dataMap[k];
            }
            else
            {
                m_confSettings->setValue(k, m_lastMap[k]);
            }
        }
    }

    if (recordObj.size())
    {
        QJsonDocument minDoc(recordObj);
        this->ConfigureChanged(m_itemMap[CONFIG_RECORD], QString::fromUtf8(minDoc.toJson(QJsonDocument::Compact).constData()));
    }

    if (auditObj.size())
    {
        QJsonDocument maxDoc(auditObj);
        this->ConfigureChanged(m_itemMap[CONFIG_AUDIT], QString::fromUtf8(maxDoc.toJson(QJsonDocument::Compact).constData()));
    }
}
