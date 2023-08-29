#ifndef XSYNCBASEDPOLLER_H
#define XSYNCBASEDPOLLER_H

#include <QHash>

#include <X11/Xdefs.h>
#include <X11/Xlib.h>
#include <X11/extensions/sync.h>
#include <xcb/xcb.h>

#include "fixx11h.h"

class XSyncBasedPoller : public QObject
{
    Q_OBJECT
public:
    static XSyncBasedPoller *instance();

    XSyncBasedPoller();
    ~XSyncBasedPoller();

    bool isAvailable();
    bool setUpPoller();
    void unloadPoller();

    bool xcbEvent(xcb_generic_event_t *event);

    QList<int> timeouts() const;

public Q_SLOTS:
    void addTimeout(int nextTimeout);
    void removeTimeout(int nextTimeout);

    int forcePollRequest();
    void catchIdleEvent();
    void stopCatchingIdleEvents();
    void simulateUserActivity();

private Q_SLOTS:
    int poll();
    void reloadAlarms();

private:
    void setAlarm(Display *dpy, XSyncAlarm *alarm, XSyncCounter counter, XSyncTestType test, XSyncValue value);

private:
    Display *m_display;
    xcb_connection_t *m_xcb_connection;

    int m_sync_event;
    XSyncCounter m_idleCounter;
    QHash<int, XSyncAlarm> m_timeoutAlarm;
    XSyncAlarm m_resetAlarm;
    bool m_available;

Q_SIGNALS:
    void resumingFromIdle();
    void timeoutReached(int msec);
};

#endif
