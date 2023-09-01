#include "general-configure.h"
#include "kiran-log/qt5-log-i.h"
#include "common-definition.h"
#include <QDir>
#include <QFileInfo>
#include <QJsonObject>
#include <QJsonDocument>
#include <QMutexLocker>
#include <QTextCodec>

GeneralConfigure::GeneralConfigure(QObject *parent)
{
    m_itemMap.insert(CONFIG_RECORD,                     GENEARL_CONFIG_RECORD);
    m_itemMap.insert(CONFIG_RECORD_FILEPATH,            KEY_RECORD_FILEPATH);
    m_itemMap.insert(CONFIG_RECORD_FILETYPE,            KEY_RECORD_FILETYPE);
    m_itemMap.insert(CONFIG_RECORD_RECORD_VIDIO,        KEY_RECORD_RECORD_VIDIO);
    m_itemMap.insert(CONFIG_RECORD_FPS,                 KEY_RECORD_FPS);
    m_itemMap.insert(CONFIG_RECORD_QUALITY,             KEY_RECORD_QUALITY);
    m_itemMap.insert(CONFIG_RECORD_RECORD_AUDIO,        KEY_RECORD_RECORD_AUDIO);
    m_itemMap.insert(CONFIG_RECORD_BITRATE,             KEY_RECORD_BITRATE);
    m_itemMap.insert(CONFIG_RECORD_WATER_PRINT,         KEY_RECORD_WATER_PRINT);
    m_itemMap.insert(CONFIG_RECORD_WATER_PRINT_TEXT,    KEY_RECORD_WATER_PRINT_TEXT);
    m_itemMap.insert(CONFIG_RECORD_TIMING_REMINDER,     KEY_RECORD_TIMING_REMINDER);
    m_itemMap.insert(CONFIG_RECORD_MIC_VOLUME,          KEY_RECORD_MIC_VOLUME);
    m_itemMap.insert(CONFIG_RECORD_SPEAKER_VOLUME,      KEY_RECORD_SPEAKER_VOLUME);
    m_itemMap.insert(CONFIG_RECORD_MIN_FREE_SPACE,      KEY_RECORD_MIN_FREE_SPACE);
    m_itemMap.insert(CONFIG_AUDIT,                      GENEARL_CONFIG_AUDIT);
    m_itemMap.insert(CONFIG_AUDIT_FILEPATH,             KEY_AUDIT_FILEPATH);
    m_itemMap.insert(CONFIG_AUDIT_FILETYPE,             KEY_AUDIT_FILETYPE);
    m_itemMap.insert(CONFIG_AUDIT_RECORD_VIDIO,         KEY_AUDIT_RECORD_VIDIO);
    m_itemMap.insert(CONFIG_AUDIT_FPS,                  KEY_AUDIT_FPS);
    m_itemMap.insert(CONFIG_AUDIT_QUALITY,              KEY_AUDIT_QUALITY);
    m_itemMap.insert(CONFIG_AUDIT_RECORD_AUDIO,         KEY_AUDIT_RECORD_AUDIO);
    m_itemMap.insert(CONFIG_AUDIT_BITRATE,              KEY_AUDIT_BITRATE);
    m_itemMap.insert(CONFIG_AUDIT_TIMING_PAUSE,         KEY_AUDIT_TIMING_PAUSE);
    m_itemMap.insert(CONFIG_AUDIT_WATER_PRINT,          KEY_AUDIT_WATER_PRINT);
    m_itemMap.insert(CONFIG_AUDIT_WATER_PRINT_TEXT,     KEY_AUDIT_WATER_PRINT_TEXT);
    m_itemMap.insert(CONFIG_AUDIT_MIN_FREE_SPACE,       KEY_AUDIT_MIN_FREE_SPACE);
    m_itemMap.insert(CONFIG_AUDIT_MAX_SAVE_DAYS,        KEY_AUDIT_MAX_SAVE_DAYS);
    m_itemMap.insert(CONFIG_AUDIT_MAX_FILE_SIZE,        KEY_AUDIT_MAX_FILE_SIZE);
    m_itemMap.insert(CONFIG_AUDIT_MAX_RECORD_PER_USER,  KEY_AUDIT_MAX_RECORD_PER_USER);

    m_confSettings = QSharedPointer<QSettings>(new QSettings(GENEARL_CONFIG_FILE, QSettings::IniFormat));
    m_confSettings->setIniCodec(QTextCodec::codecForName("UTF-8"));
    initConfig();

    m_fileWatcher = new QFileSystemWatcher();
    connect(m_fileWatcher, SIGNAL(directoryChanged(QString)), this, SLOT(onDirectoryChanged(QString)));
    m_fileWatcher->addPath(GENEARL_CONFIG_PATH);
}

GeneralConfigure &GeneralConfigure::Instance()
{
    static GeneralConfigure s_general;
    return s_general;
}

bool GeneralConfigure::setRecordConfigure(const QString &param)
{
    QJsonObject jsonObj;
    if (!parseJsonData(param, jsonObj))
        return false;

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
    QJsonObject jsonObj;
    if (!parseJsonData(param, jsonObj))
        return false;

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
        // 2: CONFIG_RECORD 和 CONFIG_AUDIT
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
    // 设置默认值
    m_lastMap.insert(m_itemMap[CONFIG_RECORD_FILEPATH],             RECORD_DEFAULT_CONFIG_FILEPATH);
    m_lastMap.insert(m_itemMap[CONFIG_RECORD_FILETYPE],             RECORD_DEFAULT_CONFIG_FILETYPE);
    m_lastMap.insert(m_itemMap[CONFIG_RECORD_RECORD_VIDIO],         RECORD_DEFAULT_CONFIG_RECORD_VIDIO);
    m_lastMap.insert(m_itemMap[CONFIG_RECORD_FPS],                  QString::number(RECORD_DEFAULT_CONFIG_FPS));
    m_lastMap.insert(m_itemMap[CONFIG_RECORD_QUALITY],              RECORD_DEFAULT_CONFIG_QUALITY);
    m_lastMap.insert(m_itemMap[CONFIG_RECORD_RECORD_AUDIO],         RECORD_DEFAULT_CONFIG_RECORD_AUDIO);
    m_lastMap.insert(m_itemMap[CONFIG_RECORD_BITRATE],              QString::number(RECORD_DEFAULT_CONFIG_BITRATE));
    m_lastMap.insert(m_itemMap[CONFIG_RECORD_WATER_PRINT],          RECORD_DEFAULT_CONFIG_WATER_PRINT);
    m_lastMap.insert(m_itemMap[CONFIG_RECORD_WATER_PRINT_TEXT],     RECORD_DEFAULT_CONFIG_WATER_PRINT_TEXT);
    m_lastMap.insert(m_itemMap[CONFIG_RECORD_TIMING_REMINDER],      QString::number(RECORD_DEFAULT_CONFIG_TIMING_REMINDER));
    m_lastMap.insert(m_itemMap[CONFIG_RECORD_MIC_VOLUME],           QString::number(RECORD_DEFAULT_CONFIG_MIC_VOLUME));
    m_lastMap.insert(m_itemMap[CONFIG_RECORD_SPEAKER_VOLUME],       QString::number(RECORD_DEFAULT_CONFIG_SPEAKER_VOLUME));
    m_lastMap.insert(m_itemMap[CONFIG_RECORD_MIN_FREE_SPACE],       QString::number(RECORD_DEFAULT_CONFIG_MIN_FREE_SPACE));

    m_lastMap.insert(m_itemMap[CONFIG_AUDIT_FILEPATH],              AUDIT_DEFAULT_CONFIG_FILEPATH);
    m_lastMap.insert(m_itemMap[CONFIG_AUDIT_FILETYPE],              AUDIT_DEFAULT_CONFIG_FILETYPE);
    m_lastMap.insert(m_itemMap[CONFIG_AUDIT_RECORD_VIDIO],          AUDIT_DEFAULT_CONFIG_RECORD_VIDIO);
    m_lastMap.insert(m_itemMap[CONFIG_AUDIT_FPS],                   QString::number(AUDIT_DEFAULT_CONFIG_FPS));
    m_lastMap.insert(m_itemMap[CONFIG_AUDIT_QUALITY],               AUDIT_DEFAULT_CONFIG_QUALITY);
    m_lastMap.insert(m_itemMap[CONFIG_AUDIT_RECORD_AUDIO],          AUDIT_DEFAULT_CONFIG_RECORD_AUDIO);
    m_lastMap.insert(m_itemMap[CONFIG_AUDIT_BITRATE],               QString::number(AUDIT_DEFAULT_CONFIG_BITRATE));
    m_lastMap.insert(m_itemMap[CONFIG_AUDIT_WATER_PRINT],           AUDIT_DEFAULT_CONFIG_WATER_PRINT);
    m_lastMap.insert(m_itemMap[CONFIG_AUDIT_WATER_PRINT_TEXT],      AUDIT_DEFAULT_CONFIG_WATER_PRINT_TEXT);
    m_lastMap.insert(m_itemMap[CONFIG_AUDIT_TIMING_PAUSE],          QString::number(AUDIT_DEFAULT_CONFIG_TIMING_PAUSE));
    m_lastMap.insert(m_itemMap[CONFIG_AUDIT_MIN_FREE_SPACE],        QString::number(AUDIT_DEFAULT_CONFIG_MIN_FREE_SPACE));
    m_lastMap.insert(m_itemMap[CONFIG_AUDIT_MAX_SAVE_DAYS],         QString::number(AUDIT_DEFAULT_CONFIG_MAX_SAVE_DAYS));
    m_lastMap.insert(m_itemMap[CONFIG_AUDIT_MAX_FILE_SIZE],         QString::number(AUDIT_DEFAULT_CONFIG_MAX_FILE_SIZE));
    m_lastMap.insert(m_itemMap[CONFIG_AUDIT_MAX_RECORD_PER_USER],   QString::number(AUDIT_DEFAULT_CONFIG_MAX_RECORD_PER_USER));
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
        return checkInteger(value, QString(QUALITY_FLUENT_VALUE).toInt(), QString(QUALITY_HD_VALUE).toInt());
    if (m_itemMap[CONFIG_RECORD_RECORD_AUDIO] == item)
        return checkRecordAudio(value);
    if (m_itemMap[CONFIG_RECORD_BITRATE] == item)
        return checkInteger(value, BITRATE_MIN_VALUE, BITRATE_MAX_VALUE);
    if (m_itemMap[CONFIG_RECORD_WATER_PRINT] == item)
        return checkEnable(value);
    if (m_itemMap[CONFIG_RECORD_WATER_PRINT_TEXT] == item)
        return checkWaterPrint(value);
    if (m_itemMap[CONFIG_RECORD_TIMING_REMINDER] == item)
        return checkInteger(value, TIMING_REMINDER_LEVEL_0, TIMING_REMINDER_LEVEL_3);
    if (m_itemMap[CONFIG_RECORD_MIC_VOLUME] == item)
        return checkInteger(value, VOLUME_MIN_VALUE, VOLUME_MAX_VALUE);
    if (m_itemMap[CONFIG_RECORD_SPEAKER_VOLUME] == item)
        return checkInteger(value, VOLUME_MIN_VALUE, VOLUME_MAX_VALUE);
    if (m_itemMap[CONFIG_RECORD_MIN_FREE_SPACE] == item)
        return checkULongLong(value, FRONT_DISK_MIN_SIZE, FRONT_DISK_MAX_SIZE);

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
        return checkInteger(value, QString(QUALITY_FLUENT_VALUE).toInt(), QString(QUALITY_HD_VALUE).toInt());
    if (m_itemMap[CONFIG_AUDIT_RECORD_AUDIO] == item)
        return checkRecordAudio(value);
    if (m_itemMap[CONFIG_AUDIT_BITRATE] == item)
        return checkInteger(value, BITRATE_MIN_VALUE, BITRATE_MAX_VALUE);
    if (m_itemMap[CONFIG_AUDIT_WATER_PRINT] == item)
        return checkEnable(value);
    if (m_itemMap[CONFIG_AUDIT_WATER_PRINT_TEXT] == item)
        return checkWaterPrint(value);
    if (m_itemMap[CONFIG_AUDIT_TIMING_PAUSE] == item)
        return checkInteger(value, TIMING_PAUSE_LEVEL_0, TIMING_PAUSE_LEVEL_3);
    if (m_itemMap[CONFIG_AUDIT_MIN_FREE_SPACE] == item)
        return checkULongLong(value, AUDIT_DISK_MIN_SIZE, AUDIT_DISK_MAX_SIZE);
    if (m_itemMap[CONFIG_AUDIT_MAX_SAVE_DAYS] == item)
        return checkInteger(value, FILE_SAVE_MIN_DAYS, FILE_SAVE_MAX_DAYS);
    if (m_itemMap[CONFIG_AUDIT_MAX_FILE_SIZE] == item)
        return checkULongLong(value, FILE_MIN_SIZE, FILE_MAX_SIZE);
    if (m_itemMap[CONFIG_AUDIT_MAX_RECORD_PER_USER] == item)
        return checkInteger(value, MAX_RECORD_PER_USER_LEVEL_0, MAX_RECORD_PER_USER_LEVEL_3);

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
    if (QString::compare(type, FILETYPE_MP4, Qt::CaseInsensitive) == 0
        || QString::compare(type, FILETYPE_MKV, Qt::CaseInsensitive) == 0
        || QString::compare(type, FILETYPE_OGV, Qt::CaseInsensitive) == 0)
    {
        return true;
    }

    KLOG_INFO() << "file type is " << type << ", and needs to be mp4 or ogv or mkv";
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
    if (CONFIG_RECORD_AUDIO_ALL == value || CONFIG_RECORD_AUDIO_NONE == value || CONFIG_RECORD_AUDIO_MIC == value || CONFIG_RECORD_AUDIO_SPEAKER == value)
        return true;

    KLOG_INFO() << "record audio is " << value << ", and needs to be all or none or mic or speaker";
    return false;
}

bool GeneralConfigure::checkWaterPrint(QString value)
{
    if (value.size() > WATER_TEXT_MAX_SIZE)
    {
        KLOG_INFO() << "record water print too big";
        return false;
    }

    return true;
}

void GeneralConfigure::rewriteConfig()
{
    QFile file(GENEARL_CONFIG_FILE);
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
    QFile file(GENEARL_CONFIG_FILE);
    if (!file.open(QIODevice::ReadOnly))
    {
        KLOG_INFO() <<  "fail to open the file: " << GENEARL_CONFIG_FILE;
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

bool GeneralConfigure::parseJsonData(const QString &param,  QJsonObject &jsonObj)
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
