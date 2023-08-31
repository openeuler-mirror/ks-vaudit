#ifndef GENERALCONFIGURE_H
#define GENERALCONFIGURE_H

#include <QMutex>
#include <QObject>
#include <QVector>
#include <QSettings>
#include <QSharedPointer>
#include <QFileSystemWatcher>

class GeneralConfigure : public QObject
{
    Q_OBJECT
public:
    GeneralConfigure(QObject *parent = 0);
    static GeneralConfigure &Instance();
    bool setRecordConfigure(const QString &param);
    bool setAuditConfigure(const QString &param);
    QString readRecordConf();
    QString readAuditConf();

private:
    ~GeneralConfigure();
    void initConfig();
    void initData();
    QString readGroupConfig(QString);
    QString fullKey(QString, QString);
    bool checkRecordParam(QString, QString);
    bool checkAuditParam(QString, QString);
    inline bool checkPath(QString);
    inline bool checkType(QString);
    inline bool checkEnable(QString);
    inline bool checkInteger(QString, int min, int max);
    inline bool checkULongLong(QString, unsigned long long min, unsigned long long max);
    inline bool checkRecordAudio(QString value);
    inline bool checkWaterPrint(QString);
    void rewriteConfig();
    bool parseJsonData(const QString &param,  QJsonObject &jsonObj);

private slots:
    void onDirectoryChanged(QString);

signals:
    void ConfigureChanged(const QString &which, const QString &changed_config);

private:
    enum ConfigureItem {
        CONFIG_RECORD,                      // 录屏
        CONFIG_RECORD_FILEPATH,             // 文件保存路径，默认~/videos/ks-vaudit
        CONFIG_RECORD_FILETYPE,             // 文件保存格式，mkv、mp4，默认mkv
        CONFIG_RECORD_RECORD_VIDIO,         // 录制视频，整数 0、1，默认1
        CONFIG_RECORD_FPS,                  // 视频帧速，整数 2 - 60，默认25
        CONFIG_RECORD_QUALITY,              // 视频清晰度，支持设置高清、清晰、流畅，对应整数 2、1、0，默认清晰
        CONFIG_RECORD_RECORD_AUDIO,         // 录制音频，all、none、mic、speaker，默认all
        CONFIG_RECORD_BITRATE,              // 音频码率，整数 1 - 256，默认128
        CONFIG_RECORD_WATER_PRINT,          // 水印开关，整数 0、1，默认0
        CONFIG_RECORD_WATER_PRINT_TEXT,     // 水印内容，0 - 64个字符，最大20个字（包括中、英文和特殊字符）
        CONFIG_RECORD_TIMING_REMINDER,      // 定时提示，整数 0、5、10、30，默认30
        CONFIG_RECORD_MIC_VOLUME,           // 麦克风音量，整数 0 - 100，默认50
        CONFIG_RECORD_SPEAKER_VOLUME,       // 扬声器音量，整数 0 - 100，默认50
        CONFIG_RECORD_MIN_FREE_SPACE,       // 最小剩余磁盘，少于就通知并停止录屏，默认1G，单位：Byte

        CONFIG_AUDIT,                       //审计
        CONFIG_AUDIT_FILEPATH,              // 文件保存路径，默认/opt
        CONFIG_AUDIT_FILETYPE,              // 文件保存格式，mkv、mp4，默认mkv
        CONFIG_AUDIT_RECORD_VIDIO,          // 录制视频，整数 0、1，默认1
        CONFIG_AUDIT_FPS,                   // 视频帧速，整数 2 - 60，默认10
        CONFIG_AUDIT_QUALITY,               // 视频清晰度，支持设置高清、清晰、流畅，对应整数 2、1、0，默认清晰
        CONFIG_AUDIT_RECORD_AUDIO,          // 录制音频，all、none、mic、speaker，默认speaker
        CONFIG_AUDIT_BITRATE,               // 音频码率，整数 1 - 256，默认128
        CONFIG_AUDIT_WATER_PRINT,           // 水印开关，整数 0、1，默认1
        CONFIG_AUDIT_WATER_PRINT_TEXT,      // 水印内容，0 - 64个字符, 不需要
        CONFIG_AUDIT_TIMING_PAUSE,          // 无操作暂停录屏，整数 0、5、10、15，默认5, 单位：分钟
        CONFIG_AUDIT_MIN_FREE_SPACE,        // 最小剩余磁盘，少于就删除最老的适配，默认10G，单位：Byte
        CONFIG_AUDIT_MAX_SAVE_DAYS,         // 最大保存时长，默认30
        CONFIG_AUDIT_MAX_FILE_SIZE,         // 单个文件最大尺寸，达到后新建文件，默认2G，单位：Byte
        CONFIG_AUDIT_MAX_RECORD_PER_USER,   // 单个用户最大录屏会话数，1个、3个、5个、不限制(0)；默认不限制
        CONFIG_MAX
    };

private:
    QString m_confFile;
    QSharedPointer<QSettings> m_confSettings;
    QFileSystemWatcher *m_fileWatcher;
    QMap<ConfigureItem, QString> m_itemMap;
    QMap<QString, QString> m_lastMap;   // 用于比较、恢复数据
    QMutex m_mutex;
};

#endif // GENERALCONFIGURE_H
