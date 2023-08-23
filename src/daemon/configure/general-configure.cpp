#include "general-configure.h"
#include <QDir>
#include <QDebug>
#include <QFileInfo>
#include <QJsonObject>
#include <QJsonDocument>
#include <QMutexLocker>

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

GeneralConfigure::GeneralConfigure(QObject *parent)
{
    m_confFile = "/etc/ks-vaudit/ks-vaudit.conf";
    m_confSettings = QSharedPointer<QSettings>(new QSettings(m_confFile, QSettings::IniFormat));
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

GeneralConfigure::~GeneralConfigure()
{
    if (m_fileWatcher)
        delete m_fileWatcher;
    m_fileWatcher = nullptr;
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

    if (fileinfo.isFile())
    {
        m_confSettings->sync();
        for (auto key : m_confSettings->allKeys())
        {
            m_lastMap.insert(key,  m_confSettings->value(key).toString());
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

    QFile file(m_confFile);
    if (file.open(QIODevice::ReadOnly))
    {
        m_data.reserve(file.size());
        QTextStream in(&file);
        while (1) {
            QString line = in.readLine();
            if (line.isNull())
                break;
            if (line.isEmpty())
                continue;

            m_data.push_back(line);
        }
        file.close();
    }
}

void GeneralConfigure::initData()
{
    m_lastMap.insert("record/FilePath", "~/ks-vaudit");
    m_lastMap.insert("record/FileType", "MP4");
    m_lastMap.insert("record/RecordVideo", "1");
    m_lastMap.insert("record/Fps", "15");
    m_lastMap.insert("record/Quality", "0");
    m_lastMap.insert("record/RecordAudio", "all");
    m_lastMap.insert("record/Bitrate", "128");
    m_lastMap.insert("record/TimingReminder", "0");
    m_lastMap.insert("record/WaterPrint", "0");
    m_lastMap.insert("record/WaterPrintText", "kylinsec");

    m_lastMap.insert("audit/FilePath", "~/ks-vaudit");
    m_lastMap.insert("audit/FileType", "MP4");
    m_lastMap.insert("audit/RecordVideo", "1");
    m_lastMap.insert("audit/Fps", "15");
    m_lastMap.insert("audit/Quality", "0");
    m_lastMap.insert("audit/RecordAudio", "all");
    m_lastMap.insert("audit/Bitrate", "128");
    m_lastMap.insert("audit/TimingReminder", "0");
    m_lastMap.insert("audit/WaterPrint", "0");
    m_lastMap.insert("audit/WaterPrintText", "kylinsec");
    m_lastMap.insert("audit/MinFreeSpace", "10737741824");
    m_lastMap.insert("audit/MaxSaveDays", "30");
    m_lastMap.insert("audit/MaxFileSize", "10737741824");
    m_lastMap.insert("audit/MaxRecordPerUser", "2");
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
        return checkInteger(value, 0, 2);
    if (m_itemMap[CONFIG_RECORD_AUDIO] == item)
        return checkRecordAudio(value);
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

bool GeneralConfigure::checkRecordAudio(QString value)
{
    if (value == "all" || value == "none" || value == "mic" || value == "speaker")
        return true;

    qWarning() << "record audio is " << value << ", and needs to be all or none or mic or speaker";
    return false;
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

void GeneralConfigure::rewriteConfig()
{
    QFile file(m_confFile);
    file.open(QFile::WriteOnly|QFile::Truncate);
    file.close();

    for (auto it = m_lastMap.begin(); it != m_lastMap.end(); it++)
    {
        m_confSettings->setValue(it.key(), it.value());
    }
    m_confSettings->sync();
}

void GeneralConfigure::onDirectoryChanged(QString)
{
    QFile file(m_confFile);
    if (!file.open(QIODevice::ReadOnly))
    {
        qWarning() << __func__ <<  "fail to open the file: " << m_confFile;
        return;
    }

    QTextStream in(&file);
    QVector<QString> data;
    data.reserve(file.size());
    while (1) {
        QString line = in.readLine();
        if (line.isNull())
            break;
        if (line.isEmpty())
            continue;

        data.push_back(line);
    }
    file.close();

    QMutexLocker locker(&m_mutex);
    if (data == m_data)
        return;

    if (m_data.size() != data.size() || data.indexOf("[audit]") != m_data.indexOf("[audit]") || data.indexOf("[record]") != m_data.indexOf("[record]"))
    {
        qWarning() << "number of options(line) changed and correction file" << m_data.size() << data.size();
        rewriteConfig();
        return;
    }

    m_confSettings->sync();
    int recordIndex = data.indexOf("[record]");
    int auditIndex = data.indexOf("[audit]");
    int index = recordIndex > auditIndex ? recordIndex : auditIndex;
    QString maxGroup = recordIndex > auditIndex ? m_itemMap[CONFIG_RECORD] : m_itemMap[CONFIG_AUDIT];
    QString minGroup = recordIndex < auditIndex ? m_itemMap[CONFIG_RECORD] : m_itemMap[CONFIG_AUDIT];
    QJsonObject minObj, maxObj;

    for (int i = 0; i < data.size(); i++)
    {
        if (data[i] != m_data[i])
        {
            auto newData = data[i].split("=");
            if (newData.size() != 2)
            {
                qWarning() << "file format change and correction file" << data[i];
                rewriteConfig();
                return;
            }

            if (i < index)
            {
                if (!checkAuditParam(newData[0], newData[1]))
                {
                    QString key = minGroup + "/" + newData[0];
                    m_confSettings->setValue(key, m_lastMap[key]);
                    data[i] = newData[0] + "=" + m_lastMap[key];
                }
                else
                {
                    minObj[newData[0]] = newData[1];
                }
            }
            else
            {
                if (!checkAuditParam(newData[0], newData[1]))
                {
                    QString key = maxGroup + "/" + newData[0];
                    m_confSettings->setValue(key, m_lastMap[key]);
                    data[i] = newData[0] + "=" + m_lastMap[key];
                }
                else
                {
                    maxObj[newData[0]] = newData[1];
                }
            }
        }
    }

    m_data = data;
    if (minObj.size())
    {
        QJsonDocument minDoc(minObj);
        this->ConfigureChanged(minGroup, QString::fromUtf8(minDoc.toJson(QJsonDocument::Compact).constData()));
    }

    if (maxObj.size())
    {
        QJsonDocument maxDoc(maxObj);
        this->ConfigureChanged(maxGroup, QString::fromUtf8(maxDoc.toJson(QJsonDocument::Compact).constData()));
    }
}
