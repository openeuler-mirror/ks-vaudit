#ifndef KYNOTIFY_H
#define KYNOTIFY_H

//定义消息类别, 针对不同的类别，给出不同的提示：启动、暂停、停止、一段固定时间后
typedef enum NotifyMessage{
    KSVAUDIT_START = 0,
    KSVAUDIT_PAUSE,
    KSVAUDIT_STOP,
    KSVAUDIT_TIMING
}NOTYFY_MESSAGE;

class KyNotify
{
public:
    KyNotify();
    static KyNotify& instance();
    void notify(NOTYFY_MESSAGE msg, int timing = 0);

private:
   ~KyNotify();
    void initNotify();
    void notify_send(const char *msg, const char *icon);
    void notify_info(const char *msg);
    void notify_warn(const char *msg);
    void notify_error(const char *msg);
};

#endif
