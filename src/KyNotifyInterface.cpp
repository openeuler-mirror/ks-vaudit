#include "KyNotifyInterface.h"
#include "kiran-log/qt5-log-i.h"

#define TIMING_REMINDER_UNIT (60 *1000)

KyNotifyInterface::KyNotifyInterface(QObject *parent) : m_timing(0)
{
    m_pTimer = new QTimer(this);
    connect(m_pTimer, SIGNAL(timeout()), this, SLOT(onTimeout()));
}

KyNotifyInterface& KyNotifyInterface::instance()
{
    static KyNotifyInterface g_inf;
	return g_inf;
}

void KyNotifyInterface::sendNotify(NOTYFY_MESSAGE msg)
{
    KLOG_INFO() << "msg:" << msg << "timing: " << m_timing;
    if (m_pTimer->isActive())
		m_pTimer->stop();
    if (msg == KSVAUDIT_START && m_timing)
        m_pTimer->start(m_timing*TIMING_REMINDER_UNIT);

    KyNotify::instance().notify(msg);
}

void KyNotifyInterface::setTiming(int timing)
{
    KLOG_INFO() << timing;
    QMutexLocker locker(&m_mutex);
	if (m_timing != timing)
	{
		if (m_pTimer->isActive())
		{
			m_pTimer->stop();
			if (timing)
				m_pTimer->start(timing*TIMING_REMINDER_UNIT);
		}
	}

    m_timing = timing;
}

void KyNotifyInterface::onTimeout()
{
    KLOG_INFO() << "call notify KSVAUDIT_TIMING";
    KyNotify::instance().notify(KSVAUDIT_TIMING, m_timing);
}

KyNotifyInterface::~KyNotifyInterface()
{
    if (m_pTimer)
        delete m_pTimer;
    m_pTimer = nullptr;
}

