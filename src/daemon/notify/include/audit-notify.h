#ifndef AUDIT_NOTIFY_H
#define AUDIT_NOTIFY_H

#include <QString>

#define NOTIFY_TIMEOUT (2 * 1000) // 2 seconds

class AuditNotify
{
public:
    AuditNotify();
    static AuditNotify& instance();
    void setParam(uid_t uid, quint64 reserveSize);
    void sendNotify();
    void clearData();

private:
    ~AuditNotify(); 
    void notify_send(const char *msg, const char *icon, int timeout, const char *userdata);

public:
    void *m_pNotify;

private:
    int m_reserveSize;
    uid_t m_uid;
};

#endif
