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
    QString readGroupConfig(QString group);
    bool checkRecordParam(QString, QString, bool *pIn = nullptr);
    bool checkAuditParam(QString, QString);
    inline bool checkPath(QString);
    inline bool checkType(QString);
    inline bool checkEnable(QString);
    inline bool checkInteger(QString, int min, int max);
    inline bool checkULongLong(QString, unsigned long long min, unsigned long long max);
    inline bool checkRecordAudio(QString value);
    inline bool checkWaterPrint(QString);
    void rewriteConfig();

private slots:
    void onDirectoryChanged(QString);

signals:
    void ConfigureChanged(const QString &which, const QString &changed_config);

private:
    enum ConfigureItem {
        CONFIG_RECORD,
        CONFIG_AUDIT,
        CONFIG_FILEPATH,                    // 文件保存路径
        CONFIG_FILETYPE,                    // 文件保存格式
        CONFIG_RECORD_VIDIO,                // 录制视频
        CONFIG_FPS,                         // 视频帧速
        CONFIG_QUALITY,                     // 视频清晰度
        CONFIG_RECORD_AUDIO,                // 录制音频
        CONFIG_BITRATE,                     // 音频码率
        CONFIG_TIMING_REMINDER,             // 定时提示
        CONFIG_WATER_PRINT,                 // 水印开关
        CONFIG_WATER_PRINT_TEXT,            // 水印内容
        CONFIG_AUDIT_MIN_FREE_SPACE,        // 最小剩余磁盘，少于就删除最老的适配
        CONFIG_AUDIT_MAX_SAVE_DAYS,         //最大保存时长
        CONFIG_AUDIT_MAX_FILE_SIZE,         // 单个文件最大尺寸，达到后新建文件
        CONFIG_AUDIT_MAX_RECORD_PER_USER,   // 单个用户最大录屏会话数
        CONFIG_MAX
    };

    QString m_confFile;
    QSharedPointer<QSettings> m_confSettings;
    QFileSystemWatcher *m_fileWatcher;
    QMap<ConfigureItem, QString> m_itemMap;
    QMap<QString, QString> m_lastMap;
    QVector<QString> m_data;
    QMutex m_mutex;
};

#endif // GENERALCONFIGURE_H
