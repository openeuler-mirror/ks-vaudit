#ifndef RECORD_NOTIFY_H
#define RECORD_NOTIFY_H

#include <QString>

class RecordNotify
{
public:
    RecordNotify();
    static RecordNotify& instance();
    void sendNotify(QString op, uid_t uid, int timing);

private:
    ~RecordNotify();
    void notify_send(const char *msg);
};

#endif
