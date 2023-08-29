#ifndef KYNOTIFY_H
#define KYNOTIFY_H

#include <QString>

#define NOTIFY_TIMEOUT (2 * 1000) // 2 seconds

class KyNotify
{
public:
    KyNotify();
    static KyNotify& instance();
    void sendNotify(QString op);
    void setTiming(int timing);
    void setRecordTime(uint64_t recordTime);
    void setContinueNotify(bool bContinue);
    bool getContinueNotify();
    void setReserveSize(quint64 value);

private:

    //定义消息类别, 针对不同的类别，给出不同的提示：启动、暂停、停止、一段固定时间后
    typedef enum NotifyMessage{
        KSVAUDIT_START = 0,
        KSVAUDIT_PAUSE,
        KSVAUDIT_STOP,
        KSVAUDIT_TIMING,
        KSVAUDIT_DISK,
    }NOTYFY_MESSAGE;

   ~KyNotify();
   void notify(NOTYFY_MESSAGE msg, int timing = 0);
    void initNotify();
    void notify_send(const char *msg, const char *icon, int timeout, const char *userdata);
    void notify_info(const char *msg, int timeout = NOTIFY_TIMEOUT, const char *userdata = nullptr);
    void notify_warn(const char *msg, int timeout = NOTIFY_TIMEOUT, const char *userdata = nullptr);
    void notify_error(const char *msg, int timeout = NOTIFY_TIMEOUT, const char *userdata = nullptr);

private:
    bool m_bStart;
    bool m_bContinue;
    int m_timing;
    int m_reserveSize;
    void *m_pDiskNotify;
};

#endif
