#ifndef COMMON_DEFINITION_H
#define COMMON_DEFINITION_H

// 录制中的文件添加后缀.tmp
#define FILE_SUFFIX_TMP                             ".tmp"
#define FILE_SUFFIX_MKV                             ".mkv"
#define FILE_SUFFIX_MP4                             ".mp4"
#define FILE_SUFFIX_OGV                             ".ogv"

// SwitchControl 函数op参数类型: OPERATE_RECORD_：录像控制相关；OPERATE_NOTIFY:前台提示; OPERATE_DISK：磁盘空间相关
#define OPERATE_RECORD_START                        "start"
#define OPERATE_RECORD_PAUSE                        "pause"
#define OPERATE_RECORD_RESTART                      "restart"
#define OPERATE_RECORD_STOP                         "stop"
#define OPERATE_RECORD_EXIT                         "exit"
#define OPERATE_RECORD_DONE                         "-done"
#define OPERATE_NOTIFY_TIMING                       "timing"
#define OPERATE_NOTIFY_ERROR                        "error"
#define OPERATE_DISK_NOTIFY                         "disk_notify"
#define OPERATE_DISK_NOTIFY_STOP                    "disk_notify_stop"
#define OPERATE_DISK_FRONT_NOTIFY                   "DiskSpace"
#define OPERATE_FRONT_PROCESS                       "process"

#define VAUDIT_ACTIVE                               "is_active"

#define GENEARL_CONFIG_PATH                         "/etc/ks-vaudit/"
#define GENEARL_CONFIG_FILE                         (GENEARL_CONFIG_PATH"ks-vaudit.conf")
#define GENEARL_CONFIG_RECORD                       "record"                // 前台录屏
#define GENEARL_CONFIG_AUDIT                        "audit"                 // 审计录屏

// 通用配置选项
#define GENEARL_CONFIG_FILEPATH                     "FilePath"              // 文件保存路径
#define GENEARL_CONFIG_FILETYPE                     "FileType"              // 文件保存格式，mkv、mp4
#define GENEARL_CONFIG_RECORD_VIDIO                 "RecordVideo"           // 录制视频，可用值 0、1
#define GENEARL_CONFIG_FPS                          "Fps"                   // 视频帧速，整数 2 - 60
#define GENEARL_CONFIG_QUALITY                      "Quality"               // 视频清晰度，支持设置高清、清晰、流畅，对应整数 2、1、0
#define GENEARL_CONFIG_RECORD_AUDIO                 "RecordAudio"           // 录制音频，all、none、mic、speaker
#define GENEARL_CONFIG_BITRATE                      "Bitrate"               // 音频码率，整数 1 - 256
#define GENEARL_CONFIG_WATER_PRINT                  "WaterPrint"            // 水印开关
#define GENEARL_CONFIG_WATER_PRINT_TEXT             "WaterPrintText"        // 水印内容，0 - 64个字符，最大20个字（包括中、英文和特殊字符）
#define GENEARL_CONFIG_TIMING_REMINDER              "TimingReminder"        // 定时提示，整数 0、5、10、30
#define GENEARL_CONFIG_MIC_VOLUME                   "MicVolume"             // 麦克风音量，整数 0 - 100
#define GENEARL_CONFIG_SPEAKER_VOLUME               "SpeakerVolume"         // 扬声器音量，整数 0 - 100
#define GENEARL_CONFIG_MIN_FREE_SPACE               "MinFreeSpace"          // 最小剩余磁盘，少于就通知并停止录屏，单位：Byte
#define GENEARL_CONFIG_TIMING_PAUSE                 "TimingPause"           // 无操作暂停录屏，可用值 0、5、10、15，单位：分钟
#define GENEARL_CONFIG_MAX_SAVE_DAYS                "MaxSaveDays"           // 最大保存时长
#define GENEARL_CONFIG_MAX_FILE_SIZE                "MaxFileSize"           // 单个文件最大尺寸，达到后新建文件，单位：Byte
#define GENEARL_CONFIG_MAX_RECORD_PER_USER          "MaxRecordPerUser"      // 单个用户最大录屏会话数，1个、3个、5个、不限制(0)；

// 文件保存格式，mkv、mp4
#define FILETYPE_MKV                                "mkv"
#define FILETYPE_MP4                                "mp4"
#define FILETYPE_OGV                                "ogv"

// 视频帧速，可用值 2 - 60
#define FPS_MIN_VALUE                               2
#define FPS_MAX_VALUE                               60

// 视频清晰度，支持设置流畅、清晰、高清，对应 0、1、2
#define QUALITY_FLUENT_VALUE                        "0"
#define QUALITY_SD_VALUE                            "1"
#define QUALITY_HD_VALUE                            "2"

// 录制音频，all、none、mic、speaker
#define CONFIG_RECORD_AUDIO_ALL                     "all"
#define CONFIG_RECORD_AUDIO_NONE                    "none"
#define CONFIG_RECORD_AUDIO_MIC                     "mic"
#define CONFIG_RECORD_AUDIO_SPEAKER                 "speaker"

// 码率，可用值 1 - 256
#define BITRATE_MIN_VALUE                           1
#define BITRATE_MAX_VALUE                           256

// 水印内容，0 - 64个字符，最大20个字（包括中、英文和特殊字符）
#define WATER_TEXT_MAX_SIZE                         64

// 定时提醒 分钟
#define TIMING_REMINDER_LEVEL_0                     0
#define TIMING_REMINDER_LEVEL_1                     5
#define TIMING_REMINDER_LEVEL_2                     10
#define TIMING_REMINDER_LEVEL_3                     30

// 麦克风、扬声器音量，可用值 0 - 100
#define VOLUME_MIN_VALUE                            0
#define VOLUME_MID_VALUE                            50
#define VOLUME_MAX_VALUE                            100

// 无操作暂停录屏 分钟
#define TIMING_PAUSE_LEVEL_0                        0
#define TIMING_PAUSE_LEVEL_1                        5
#define TIMING_PAUSE_LEVEL_2                        10
#define TIMING_PAUSE_LEVEL_3                        15

// 最大保存时长
#define FILE_SAVE_MIN_DAYS                          1
#define FILE_SAVE_MAX_DAYS                          (INT_MAX)
#define SAVE_MAX_DAYS_LEVEL_0                       3
#define SAVE_MAX_DAYS_LEVEL_1                       7
#define SAVE_MAX_DAYS_LEVEL_2                       15
#define SAVE_MAX_DAYS_LEVEL_3                       30

// 单个用户最大录屏会话数, 0: 不限制
#define MAX_RECORD_PER_USER_LEVEL_0                 0
#define MAX_RECORD_PER_USER_LEVEL_1                 1
#define MAX_RECORD_PER_USER_LEVEL_2                 3
#define MAX_RECORD_PER_USER_LEVEL_3                 5

// 字节单位
#define MEGABYTE                                    (1048576ULL)
#define GIGABYTE                                    (1073741824ULL)
// 单个文件最大尺寸
#define FILE_MIN_SIZE                               (10 * MEGABYTE)
#define FILE_MAX_SIZE                               (16 * GIGABYTE)
// 前台录屏最小磁盘剩余空间
#define FRONT_DISK_MIN_SIZE                         (0ULL)
#define FRONT_DISK_MAX_SIZE                         (ULLONG_MAX)
// 后台审计录屏最小磁盘剩余空间
#define AUDIT_DISK_MIN_SIZE                         (1 * GIGABYTE)
#define AUDIT_DISK_MAX_SIZE                         (ULLONG_MAX)
#define AUDIT_DISK_LEVEL_0                          (5 * GIGABYTE)
#define AUDIT_DISK_LEVEL_1                          (10 * GIGABYTE)
#define AUDIT_DISK_LEVEL_2                          (15 * GIGABYTE)
#define AUDIT_DISK_LEVEL_3                          (20 * GIGABYTE)

// 开关
#define SWITCH_DISABLE                              "0"
#define SWITCH_ENABLE                               "1"

// 默认配置
#define RECORD_DEFAULT_CONFIG_FILEPATH              "~/videos/ks-vaudit"
#define RECORD_DEFAULT_CONFIG_FILETYPE              FILETYPE_MKV
#define RECORD_DEFAULT_CONFIG_RECORD_VIDIO          SWITCH_ENABLE
#define RECORD_DEFAULT_CONFIG_FPS                   25
#define RECORD_DEFAULT_CONFIG_QUALITY               QUALITY_SD_VALUE
#define RECORD_DEFAULT_CONFIG_RECORD_AUDIO          CONFIG_RECORD_AUDIO_ALL
#define RECORD_DEFAULT_CONFIG_BITRATE               128
#define RECORD_DEFAULT_CONFIG_WATER_PRINT           SWITCH_DISABLE
#define RECORD_DEFAULT_CONFIG_WATER_PRINT_TEXT      ""
#define RECORD_DEFAULT_CONFIG_TIMING_REMINDER       TIMING_REMINDER_LEVEL_3
#define RECORD_DEFAULT_CONFIG_MIC_VOLUME            50
#define RECORD_DEFAULT_CONFIG_SPEAKER_VOLUME        50
#define RECORD_DEFAULT_CONFIG_MIN_FREE_SPACE        GIGABYTE

#define AUDIT_DEFAULT_CONFIG_FILEPATH               "/opt/ks-vaudit"
#define AUDIT_DEFAULT_CONFIG_FILETYPE               FILETYPE_MKV
#define AUDIT_DEFAULT_CONFIG_RECORD_VIDIO           SWITCH_ENABLE
#define AUDIT_DEFAULT_CONFIG_FPS                    10
#define AUDIT_DEFAULT_CONFIG_QUALITY                QUALITY_SD_VALUE
#define AUDIT_DEFAULT_CONFIG_RECORD_AUDIO           CONFIG_RECORD_AUDIO_NONE
#define AUDIT_DEFAULT_CONFIG_BITRATE                128
#define AUDIT_DEFAULT_CONFIG_WATER_PRINT            SWITCH_ENABLE
#define AUDIT_DEFAULT_CONFIG_WATER_PRINT_TEXT       ""
#define AUDIT_DEFAULT_CONFIG_TIMING_PAUSE           TIMING_PAUSE_LEVEL_1
#define AUDIT_DEFAULT_CONFIG_MIN_FREE_SPACE         (10 * GIGABYTE)
#define AUDIT_DEFAULT_CONFIG_MAX_SAVE_DAYS          30
#define AUDIT_DEFAULT_CONFIG_MAX_FILE_SIZE          (2 * GIGABYTE)
#define AUDIT_DEFAULT_CONFIG_MAX_RECORD_PER_USER    MAX_RECORD_PER_USER_LEVEL_0

#define VAUDIT_LOCAL_IP                             "127.0.0.1"

// 配置文件选项
#define KEY_RECORD_FILEPATH                         (GENEARL_CONFIG_RECORD "/" GENEARL_CONFIG_FILEPATH)
#define KEY_RECORD_FILETYPE                         (GENEARL_CONFIG_RECORD "/" GENEARL_CONFIG_FILETYPE)
#define KEY_RECORD_RECORD_VIDIO                     (GENEARL_CONFIG_RECORD "/" GENEARL_CONFIG_RECORD_VIDIO)
#define KEY_RECORD_FPS                              (GENEARL_CONFIG_RECORD "/" GENEARL_CONFIG_FPS)
#define KEY_RECORD_QUALITY                          (GENEARL_CONFIG_RECORD "/" GENEARL_CONFIG_QUALITY)
#define KEY_RECORD_RECORD_AUDIO                     (GENEARL_CONFIG_RECORD "/" GENEARL_CONFIG_RECORD_AUDIO)
#define KEY_RECORD_BITRATE                          (GENEARL_CONFIG_RECORD "/" GENEARL_CONFIG_BITRATE)
#define KEY_RECORD_WATER_PRINT                      (GENEARL_CONFIG_RECORD "/" GENEARL_CONFIG_WATER_PRINT)
#define KEY_RECORD_WATER_PRINT_TEXT                 (GENEARL_CONFIG_RECORD "/" GENEARL_CONFIG_WATER_PRINT_TEXT)
#define KEY_RECORD_TIMING_REMINDER                  (GENEARL_CONFIG_RECORD "/" GENEARL_CONFIG_TIMING_REMINDER)
#define KEY_RECORD_MIC_VOLUME                       (GENEARL_CONFIG_RECORD "/" GENEARL_CONFIG_MIC_VOLUME)
#define KEY_RECORD_SPEAKER_VOLUME                   (GENEARL_CONFIG_RECORD "/" GENEARL_CONFIG_SPEAKER_VOLUME)
#define KEY_RECORD_MIN_FREE_SPACE                   (GENEARL_CONFIG_RECORD "/" GENEARL_CONFIG_MIN_FREE_SPACE)

#define KEY_AUDIT_FILEPATH                          (GENEARL_CONFIG_AUDIT "/" GENEARL_CONFIG_FILEPATH)
#define KEY_AUDIT_FILETYPE                          (GENEARL_CONFIG_AUDIT "/" GENEARL_CONFIG_FILETYPE)
#define KEY_AUDIT_RECORD_VIDIO                      (GENEARL_CONFIG_AUDIT "/" GENEARL_CONFIG_RECORD_VIDIO)
#define KEY_AUDIT_FPS                               (GENEARL_CONFIG_AUDIT "/" GENEARL_CONFIG_FPS)
#define KEY_AUDIT_QUALITY                           (GENEARL_CONFIG_AUDIT "/" GENEARL_CONFIG_QUALITY)
#define KEY_AUDIT_RECORD_AUDIO                      (GENEARL_CONFIG_AUDIT "/" GENEARL_CONFIG_RECORD_AUDIO)
#define KEY_AUDIT_BITRATE                           (GENEARL_CONFIG_AUDIT "/" GENEARL_CONFIG_BITRATE)
#define KEY_AUDIT_TIMING_PAUSE                      (GENEARL_CONFIG_AUDIT "/" GENEARL_CONFIG_TIMING_PAUSE)
#define KEY_AUDIT_WATER_PRINT                       (GENEARL_CONFIG_AUDIT "/" GENEARL_CONFIG_WATER_PRINT)
#define KEY_AUDIT_WATER_PRINT_TEXT                  (GENEARL_CONFIG_AUDIT "/" GENEARL_CONFIG_WATER_PRINT_TEXT)
#define KEY_AUDIT_MIN_FREE_SPACE                    (GENEARL_CONFIG_AUDIT "/" GENEARL_CONFIG_MIN_FREE_SPACE)
#define KEY_AUDIT_MAX_SAVE_DAYS                     (GENEARL_CONFIG_AUDIT "/" GENEARL_CONFIG_MAX_SAVE_DAYS)
#define KEY_AUDIT_MAX_FILE_SIZE                     (GENEARL_CONFIG_AUDIT "/" GENEARL_CONFIG_MAX_FILE_SIZE)
#define KEY_AUDIT_MAX_RECORD_PER_USER               (GENEARL_CONFIG_AUDIT "/" GENEARL_CONFIG_MAX_RECORD_PER_USER)

#define UI_INDEX_LEVEL_0                            0
#define UI_INDEX_LEVEL_1                            1
#define UI_INDEX_LEVEL_2                            2
#define UI_INDEX_LEVEL_3                            3
#define UI_FILETYPE_MKV                             "MKV"
#define UI_FILETYPE_MP4                             "MP4"
#define UI_FILETYPE_OGV                             "OGV"
#define UI_SYSADM_USERNAME                          "sysadm"
#define UI_SECADM_USERNAME                          "secadm"
#define UI_AUDADM_USERNAME                          "audadm"
#define DBUS_TIMEOUT_MS                             5000
#define DBUS_LICENSE_DESTIONATION                   "com.kylinsec.Kiran.LicenseManager"
#define DBUS_LICENSE_OBJECT_PATH                    "/com/kylinsec/Kiran/LicenseManager"
#define DBUS_LICENSE_OBJECT_VAUDIT_NAME             "KSVAUDITRECORD"
#define DBUS_LICENSE_VAUDIT_PATH                    "/com/kylinsec/Kiran/LicenseObject/KSVAUDITRECORD"
#define DBUS_LICENSE_INTERFACE                      "com.kylinsec.Kiran.LicenseObject"
#define DBUS_LICENSE_METHOD_OBJECK                  "GetLicenseObject"
#define DBUS_LICENSE_METHOD_GETLICENSE              "GetLicense"
#define DBUS_LICENSE_METHOD_ACTIVATE                "ActivateByActivationCode"
#define DBUS_LICENSE_ACTIVATION_CODE                "activation_code"
#define DBUS_LICENSE_ACTIVATION_STATUS              "activation_status"
#define DBUS_LICENSE_ACTIVATION_TIME                "activation_time"
#define DBUS_LICENSE_MACHINE_CODE                   "machine_code"
#define DBUS_LICENSE_EXPIRED_TIME                   "expired_time"

#endif
