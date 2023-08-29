#include "kidletime.h"
#include "kiran-log/qt5-log-i.h"

KIdleTime &KIdleTime::instance()
{
    static KIdleTime s_KIdleTime;
    return s_KIdleTime;
}

KIdleTime::KIdleTime(): m_catchResume(false), m_currentId(0)
{
    m_poller = XSyncBasedPoller::instance();
    if (!m_poller || !m_poller->isAvailable())
    {
        KLOG_ERROR() << "call XSyncBasedPoller err" << "m_poller:" << m_poller;
        if (m_poller)
            delete m_poller;
        exit(1);
    }

    KLOG_INFO() << "m_poller:" << m_poller << "isAvailable:" << m_poller->isAvailable();
    m_poller->setUpPoller();

    connect(m_poller, &XSyncBasedPoller::resumingFromIdle, this, &KIdleTime::onResumingFromIdle);
    connect(m_poller, &XSyncBasedPoller::timeoutReached, this, &KIdleTime::onTimeoutReached);

}

int KIdleTime::idleTime() const
{
    KLOG_DEBUG() << "forcePollRequest:" << m_poller->forcePollRequest();
    return m_poller->forcePollRequest();
}

int KIdleTime::addIdleTimeout(int msec)
{
    m_poller->addTimeout(msec);
    ++m_currentId;
    m_associations[m_currentId] = msec;
    KLOG_DEBUG() << "m_currentId:" << m_currentId << "msec:" << msec;
    return m_currentId;
}

int KIdleTime::addIdleTimeout(std::chrono::milliseconds msec)
{
    return addIdleTimeout(int(msec.count()));
}

void KIdleTime::removeIdleTimeout(int identifier)
{
#if 1
    if (!m_associations.contains(identifier))
    {
        KLOG_DEBUG() << "not find:" << identifier;
        return;
    }

    int msec = m_associations[identifier];
    m_associations.remove(identifier);

    if (!m_associations.values().contains(msec))
    {
        m_poller->removeTimeout(msec);
        KLOG_DEBUG() << "removeTimeout" << msec;
    }
#else
    const auto it = m_associations.constFind(identifier);
    if (it == m_associations.cend())
    {
        KLOG_DEBUG() << "not find:" << identifier;
        return;
    }

    const int msec = it.value();
    m_associations.erase(it);

    bool isFound = std::any_of(m_associations.cbegin(), m_associations.cend(), [msec](int i) {
        KLOG_DEBUG() << "i:" << i << "msec:" << msec;
        return i == msec;
    });

    KLOG_DEBUG() << "found" << msec << ":" << isFound;
    if (!isFound)
        m_poller->removeTimeout(msec);
#endif
}

void KIdleTime::removeAllIdleTimeouts()
{
#if 1
    QHash< int, int >::iterator it = m_associations.begin();
    QSet<int> removed;
    removed.reserve(m_associations.size());
    while (it != m_associations.end())
    {
        int msec = m_associations[it.key()];
        it = m_associations.erase(it);
        if (!removed.contains(msec))
        {
            m_poller->removeTimeout(msec);
            removed.insert(msec);
        }
    }
#else
    std::vector<int> removed;

    for (auto it = m_associations.cbegin(); it != m_associations.cend(); ++it)
    {
        const int msec = it.value();
        const bool alreadyIns = std::find(removed.cbegin(), removed.cend(), msec) != removed.cend();
        KLOG_DEBUG() << "msec:" << msec << "alreadyIns:" << alreadyIns;
        if (!alreadyIns)
        {
            removed.push_back(msec);
            m_poller->removeTimeout(msec);
        }
    }

    m_associations.clear();
#endif
}

void KIdleTime::catchNextResumeEvent()
{
    if (!m_catchResume)
    {
       m_catchResume = true;
       m_poller->catchIdleEvent();
    }
}

void KIdleTime::simulateUserActivity()
{
    m_poller->simulateUserActivity();
}

KIdleTime::~KIdleTime()
{
    if (m_poller)
    {
        m_poller->unloadPoller();
        m_poller->deleteLater();
        m_poller = nullptr;
    }
}

void KIdleTime::onResumingFromIdle()
{
    KLOG_DEBUG() << "catchResume:" << m_catchResume;
    if (m_catchResume)
    {
        emit resumingFromIdle();
        m_catchResume = false;
        m_poller->stopCatchingIdleEvents();
    }
}

void KIdleTime::onTimeoutReached(int msec)
{
    KLOG_DEBUG() << "msec:" << msec;
    const auto listKeys = m_associations.keys(msec);
    for (const auto key : listKeys)
    {
        KLOG_DEBUG() << "onTimeoutReached:" << key << msec;
        emit timeoutReached(key, msec);
    }
}

void KIdleTime::printTimeouts()
{
    KLOG_DEBUG() << "m_associations:";
    QHash<int,int>::const_iterator it = m_associations.constBegin();
    while (it != m_associations.constEnd()) {
        KLOG_DEBUG() << "key:" << it.key() << "value:" << it.value();
        ++it;
    }
}

#if 0
QHash<int, int> KIdleTime::idleTimeouts() const
{
    return associations;
}
#endif
