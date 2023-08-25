#ifndef KYNOTIFY_INTERFACE_H
#define KYNOTIFY_INTERFACE_H

#include "KyNotify.h"
#include <QTimer>
#include <QObject>
#include <QMutex>

class KyNotifyInterface : public QObject
{
    Q_OBJECT
public:
    KyNotifyInterface(QObject *parent = 0);
    static KyNotifyInterface& instance();
    void sendNotify(NOTYFY_MESSAGE msg);
    void setTiming(int timing);

private slots:
    void onTimeout();

private:
    ~KyNotifyInterface();

private:
    int m_timing;
    QTimer *m_pTimer;
    QMutex m_mutex;
};


#endif