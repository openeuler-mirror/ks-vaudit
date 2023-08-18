#include "general-configure.h"
#include <QDir>
#include <QDebug>
#include <QFileInfo>
#include <QJsonObject>
#include <QJsonDocument>

//conatin MIN, contain MAX
#define FPS_MIN_VALUE               2
#define FPS_MAX_VALUE               60
#define BITRATE_MIN_VALUE           1
#define BITRATE_MAX_VALUE           256
#define TIMING_REMINDER_MAX_VALUE   60
#define WATER_PRINT_MAX_SIZE        64
#define SAVE_DAYS_MIN_VALUE         1
#define SAVE_DAYS_MAX_VALUE         (INT_MAX)
#define RECORD_PER_USER_MIN_VALUE   1
#define RECORD_PER_USER_MAX_VALUE   (INT_MAX)
#define FREE_SPACE_MIN_SIZE         (1*1024*1024*1024ULL)
#define FREE_SPACE_MAX_SIZE         (ULLONG_MAX)
#define FILE_MIN_SIZE               (1024ULL)
#define FILE_MAX_SIZE               (4*1024*1024*1024ULL)

GeneralConfigure::GeneralConfigure()
{
    m_confFile = "/etc/ks-vaudit/ks-vaudit.conf";
    m_confSettings = QSharedPointer<QSettings>(new QSettings(m_confFile, QSettings::NativeFormat));
    initConfig();
}

GeneralConfigure &GeneralConfigure::Instance()
{
    static GeneralConfigure s_general;
    return s_general;
}

bool GeneralConfigure::setRecordConfigure(const QString &param)
{
    QString str = param;
    QJsonDocument jsonDocument = QJsonDocument::fromJson(str.toLatin1());
    if (!jsonDocument.isObject())
        return false;

    QJsonObject jsonObj = jsonDocument.object();
    for (auto k : jsonObj.keys())
    {
        if (!checkRecordParam(k, jsonObj[k].toString()))
            return false;
    }

    m_confSettings->beginGroup(m_itemMap[CONFIG_RECORD]);
    for (auto k : jsonObj.keys())
    {
        m_confSettings->setValue(k, jsonObj[k].toString());
        qWarning() << "write " << k << jsonObj[k].toString();
    }

    m_confSettings->endGroup();

    return true;
}

bool GeneralConfigure::setAuditConfigure(const QString &param)
{
    QString str = param;
    QJsonDocument jsonDocument = QJsonDocument::fromJson(str.toLatin1());
    if (!jsonDocument.isObject())
        return false;

    QJsonObject jsonObj = jsonDocument.object();
    for (auto k : jsonObj.keys())
    {
        if (!checkAuditParam(k, jsonObj[k].toString()))
            return false;
    }

    m_confSettings->beginGroup(m_itemMap[CONFIG_AUDIT]);
    for (auto k : jsonObj.keys())
    {
        m_confSettings->setValue(k, jsonObj[k].toString());
        qWarning() << "write " << k << jsonObj[k].toString();
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

void GeneralConfigure::initConfig()
{
    m_itemMap.insert(CONFIG_RECORD, "record");
    m_itemMap.insert(CONFIG_AUDIT, "audit");

    m_itemMap.insert(CONFIG_FILEPATH, "FilePath");
    m_itemMap.insert(CONFIG_FILETYPE, "FileType");
    m_itemMap.insert(CONFIG_RECORD_VIDIO, "RecordVideo");
    m_itemMap.insert(CONFIG_FPS, "Fps");
    m_itemMap.insert(CONFIG_QUALITY, "Quality");
    m_itemMap.insert(CONFIG_RECORD_AUDIO, "RecordAudio");
    m_itemMap.insert(CONFIG_BITRATE, "Bitrate");
    m_itemMap.insert(CONFIG_TIMING_REMINDER, "TimingReminder");
    m_itemMap.insert(CONFIG_WATER_PRINT, "WaterPrint");
    m_itemMap.insert(CONFIG_WATER_PRINT_TEXT, "WaterPrintText");

    m_itemMap.insert(CONFIG_AUDIT_MIN_FREE_SPACE, "MinFreeSpace");
    m_itemMap.insert(CONFIG_AUDIT_MAX_SAVE_DAYS, "MaxSaveDays");
    m_itemMap.insert(CONFIG_AUDIT_MAX_FILE_SIZE, "MaxFileSize");
    m_itemMap.insert(CONFIG_AUDIT_MAX_RECORD_PER_USER, "MaxRecordPerUser");

    QFileInfo fileinfo(m_confSettings->fileName());
    qWarning() << m_confSettings->fileName() << fileinfo.isFile();
    if (!fileinfo.isFile())
    {
        m_confSettings->beginGroup(m_itemMap[CONFIG_RECORD]);
        m_confSettings->setValue(m_itemMap[CONFIG_FILEPATH], "~/ks-vaudit");
        m_confSettings->setValue(m_itemMap[CONFIG_FILETYPE],  "mp4");
        m_confSettings->setValue(m_itemMap[CONFIG_RECORD_VIDIO], "1");
        m_confSettings->setValue(m_itemMap[CONFIG_FPS], "15");
        m_confSettings->setValue(m_itemMap[CONFIG_QUALITY],  "superfast");
        m_confSettings->setValue(m_itemMap[CONFIG_RECORD_AUDIO], "1");
        m_confSettings->setValue(m_itemMap[CONFIG_BITRATE], "18");
        m_confSettings->setValue(m_itemMap[CONFIG_TIMING_REMINDER], "0");
        m_confSettings->setValue(m_itemMap[CONFIG_WATER_PRINT], "0");
        m_confSettings->setValue(m_itemMap[CONFIG_WATER_PRINT_TEXT], "kylinsec");
        m_confSettings->endGroup();

        m_confSettings->beginGroup(m_itemMap[CONFIG_AUDIT]);
        m_confSettings->setValue(m_itemMap[CONFIG_FILEPATH], "/opt/ks-vaudit/");
        m_confSettings->setValue(m_itemMap[CONFIG_FILETYPE],  "mp4");
        m_confSettings->setValue(m_itemMap[CONFIG_RECORD_VIDIO], "1");
        m_confSettings->setValue(m_itemMap[CONFIG_FPS], "15");
        m_confSettings->setValue(m_itemMap[CONFIG_QUALITY],  "superfast");
        m_confSettings->setValue(m_itemMap[CONFIG_RECORD_AUDIO], "1");
        m_confSettings->setValue(m_itemMap[CONFIG_BITRATE], "18");
        m_confSettings->setValue(m_itemMap[CONFIG_TIMING_REMINDER], "0");
        m_confSettings->setValue(m_itemMap[CONFIG_WATER_PRINT], "0");
        m_confSettings->setValue(m_itemMap[CONFIG_WATER_PRINT_TEXT], "kylinsec");
        m_confSettings->setValue(m_itemMap[CONFIG_AUDIT_MIN_FREE_SPACE], "10G");
        m_confSettings->setValue(m_itemMap[CONFIG_AUDIT_MAX_SAVE_DAYS],  "30");
        m_confSettings->setValue(m_itemMap[CONFIG_AUDIT_MAX_FILE_SIZE], "2G");
        m_confSettings->setValue(m_itemMap[CONFIG_AUDIT_MAX_RECORD_PER_USER], "2");
        m_confSettings->endGroup();
        m_confSettings->sync();
    }
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

bool GeneralConfigure::checkRecordParam(QString item, QString value, bool *pIn)
{
    if (m_itemMap[CONFIG_FILEPATH] == item)
        return checkPath(value);
    if (m_itemMap[CONFIG_FILETYPE] == item)
        return checkType(value);
    if (m_itemMap[CONFIG_RECORD_VIDIO] == item)
        return checkEnable(value);
    if (m_itemMap[CONFIG_FPS] == item)
        return checkInteger(value, FPS_MIN_VALUE, FPS_MAX_VALUE);
    if (m_itemMap[CONFIG_QUALITY] == item)
        return checkQuality(value);
    if (m_itemMap[CONFIG_RECORD_AUDIO] == item)
        return checkEnable(value);
    if (m_itemMap[CONFIG_BITRATE] == item)
        return checkInteger(value, BITRATE_MIN_VALUE, BITRATE_MAX_VALUE);
    if (m_itemMap[CONFIG_TIMING_REMINDER] == item)
        return checkInteger(value, 0, TIMING_REMINDER_MAX_VALUE);
    if (m_itemMap[CONFIG_WATER_PRINT] == item)
        return checkEnable(value);
    if (m_itemMap[CONFIG_WATER_PRINT_TEXT] == item)
        return checkWaterPrint(value);

    if (pIn)
        *pIn = false;
    else
        qWarning() << item << " is not in the record configure options";
    return false;
}

bool GeneralConfigure::checkAuditParam(QString item, QString value)
{
    bool isRecord = true;
    bool ret = checkRecordParam(item, value, &isRecord);
    if (ret || (!ret && isRecord))
        return ret;

    if (m_itemMap[CONFIG_AUDIT_MIN_FREE_SPACE] == item)
        return checkULongLong(value, FREE_SPACE_MIN_SIZE, FREE_SPACE_MAX_SIZE);
    if (m_itemMap[CONFIG_AUDIT_MAX_SAVE_DAYS] == item)
        return checkInteger(value, SAVE_DAYS_MIN_VALUE, SAVE_DAYS_MAX_VALUE);
    if (m_itemMap[CONFIG_AUDIT_MAX_FILE_SIZE] == item)
        return checkULongLong(value, FILE_MIN_SIZE, FILE_MAX_SIZE);
    if (m_itemMap[CONFIG_AUDIT_MAX_RECORD_PER_USER] == item)
        return checkInteger(value, RECORD_PER_USER_MIN_VALUE, RECORD_PER_USER_MAX_VALUE);

    qWarning() << item << " is not in the audit configure options";
    return false;
}

bool GeneralConfigure::checkPath(QString path)
{
    QDir dir(path);
    if (dir.exists())
        return true;

    qWarning() << "path: " << path << " is not exist";
    return false;
}

bool GeneralConfigure::checkType(QString type)
{
    if (type == "mp4" || type == "MP4" || type == "ogv" || type == "OGV")
        return true;

    qWarning() << "file type is " << type << ", and needs to be mp4 or ogv";
    return false;
}

bool GeneralConfigure::checkEnable(QString enable)
{
    bool ok{};
    int tmp = enable.toInt(&ok);
    if (!ok || (tmp != 0 && tmp != 1))
    {
        qWarning() << enable << " is err, needs to be 0 or 1";
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
        qWarning() << value << " is err, needs to be " << min << " - " << max;
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
        qWarning() << value << " is err, needs to be " << min << " - " << max;
        return false;
    }

    return true;
}

bool GeneralConfigure::checkQuality(QString value)
{
    if (value.isEmpty())
    {
        qWarning() << "record quality is empty, err";
        return false;
    }

    return true;
}

bool GeneralConfigure::checkWaterPrint(QString value)
{
    if (value.size() > WATER_PRINT_MAX_SIZE)
    {
        qWarning() << "record water print too big";
        return false;
    }

    return true;
}
