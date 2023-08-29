#ifndef KIDLETIME_H
#define KIDLETIME_H

#include <QObject>
#include "xsyncbasedpoller.h"
#include <QHash>
#include <chrono>

class KIdleTime : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(KIdleTime)

public:
    static KIdleTime &instance();
    ~KIdleTime() override;
    int idleTime() const;
    int addIdleTimeout(int msec);
    int addIdleTimeout(std::chrono::milliseconds msec);
    void removeIdleTimeout(int identifier);
    void removeAllIdleTimeouts();
    void catchNextResumeEvent();
    void simulateUserActivity();
    void printTimeouts();

private:
    KIdleTime();

private slots:
    void onResumingFromIdle();
    void onTimeoutReached(int msec);

signals:
    void resumingFromIdle();
    void timeoutReached(int identifier, int msec); // clazy:exclude=overloaded-signal

private:
    bool m_catchResume;
    int m_currentId;
    QHash<int, int> m_associations;
    XSyncBasedPoller *m_poller;
};

#endif /* KIDLETIME_H */
