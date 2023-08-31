#include "KyNotify.h"
#include <glib/gi18n.h>
#include <locale.h>
#include <libnotify/notify.h>
#include <pwd.h>
#include "kiran-log/qt5-log-i.h"

#define GETTEXT_DOMAIN "kylin"
#define LICENSE_LOCALEDIR "/usr/share/locale"

static void notification_closed(NotifyNotification *pNotify, gpointer *userdata)
{
	KLOG_INFO() << "close notify window";
	g_object_unref(pNotify);
	if (userdata)
		KyNotify::instance().closeWindowDeal((char *)userdata);
}

KyNotify::KyNotify() : m_bStart(false), m_timing(0), m_bContinue(false), m_reserveSize(10),
					m_lastSecond(0), m_pDiskNotify(nullptr), m_uid(0), m_source_uid(0)
{
}

KyNotify& KyNotify::instance()
{
	static KyNotify g_notify;
	return g_notify;
}

void KyNotify::sendNotify(QString op)
{
	if (op == "disk_notify")
	{
		if (m_bContinue)
			return;

		KLOG_INFO() << "disk space notify init";
		if (!notify_init(_("kylin verify")))
		{
			KLOG_ERROR() << "Failed to initialize libnotify";
			return;
		}
		m_bContinue = true;
		notify(KSVAUDIT_DISK);
	}
	else
	{
		if (!notify_init(_("kylin verify")))
		{
			KLOG_ERROR() << "Failed to initialize libnotify";
			return;
		}

		if (op == "start")
		{
			notify(KSVAUDIT_START);
			m_bStart = true;
		}
		else if (op == "pause")
		{
			notify(KSVAUDIT_PAUSE);
		}
		else if (op == "restart")
		{
			notify(KSVAUDIT_START);
		}
		else if (op == "stop")
		{
			notify(KSVAUDIT_STOP);
			m_bStart = false;
		}

		notify_uninit();
	}
}

void KyNotify::setTiming(int timing)
{
	m_timing = timing;
}

void KyNotify::setRecordTime(uint64_t recordTime)
{
	if (recordTime && m_bStart && m_timing)
	{
		unsigned int time = (recordTime + 500000) / 1000000;
		int second = time % 60;
		// m_lastSecond == 0: 上次已经提示过或者第一次进来；second != 0 && m_lastSecond < second: 解决跳秒不提示
		if (m_lastSecond == 0 || (second != 0 && m_lastSecond < second))
		{
			m_lastSecond = second;
			return;
		}

		int minute = time / 60;
		if (minute % m_timing == 0)
			notify(KSVAUDIT_TIMING, minute);
		KLOG_DEBUG() << "notify m_timing:" << m_timing << "recordTime:" << recordTime << "time:" << time << "minute:" << minute
					 << "reminder:" << (minute % m_timing) << "second:" << second << "m_lastSecond" << m_lastSecond;
		m_lastSecond = 0;
	}
}

void KyNotify::setContinueNotify(bool bContinue)
{
	KLOG_DEBUG() << "bContinue:" << bContinue << ", last bContinue:" << m_bContinue;
	m_bContinue = bContinue;
	if (!m_bContinue && m_pDiskNotify)
	{
		KLOG_INFO() << "call notify_notification_close and unint";
		notify_notification_close((NotifyNotification *)m_pDiskNotify, NULL);
		g_object_unref(m_pDiskNotify);
		m_pDiskNotify = nullptr;
		notify_uninit();
	}
}

bool KyNotify::getContinueNotify()
{
	return m_bContinue;
}

void KyNotify::setReserveSize(quint64 value)
{
	m_reserveSize = value / 1073741824 + 10;
	KLOG_INFO() << "m_reserveSize:" << m_reserveSize << "value:" << value;
	if (m_bContinue)
	{
		KLOG_DEBUG() << "modify min diskspace, call notify_notification_close";
		notify_notification_close((NotifyNotification *)m_pDiskNotify, NULL);
		g_object_unref(m_pDiskNotify);
		m_pDiskNotify = nullptr;
		notify(KSVAUDIT_DISK);
	}
}

void KyNotify::closeWindowDeal(const char *data)
{
	if (getContinueNotify())
	{
		KLOG_INFO() << "open notify window, userdata:"<< data;
		if (data == "disk_notify")
		{
			notify(KSVAUDIT_DISK);
		}
	}
}

void KyNotify::setUser(QString userName)
{
	// 获取euid, 发送提示前需要设置对应的euid，否则会失败
	m_source_uid = getuid();
	if (userName.isEmpty())
	{
		m_uid = m_source_uid;
	}
	else
	{
		struct passwd * pw = getpwnam(userName.toStdString().c_str());
		m_uid = pw->pw_uid;
		KLOG_INFO() << "userName:" << userName << "uid:" << pw->pw_uid << "gid" << pw->pw_gid;
	}

	KLOG_INFO() << "m_source_uid:" << m_source_uid << "m_uid:" << m_uid;
}

void KyNotify::notify(NOTYFY_MESSAGE msg, int timing)
{
	setreuid(m_uid, m_uid);
	setlocale(LC_ALL, "");
	bindtextdomain(GETTEXT_DOMAIN, LICENSE_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_DOMAIN, "UTF-8");
	textdomain(GETTEXT_DOMAIN);
	char *notify_message = NULL;

	switch (msg)
	{
		case KSVAUDIT_START:
			{
				notify_message = _("<b>\t已开始录屏</b>\n");
				notify_info(notify_message);
				break;
			}
		case KSVAUDIT_PAUSE:
			{
				notify_message = _("<b>\t已暂停录屏</b>\n");
				notify_info(notify_message);
				break;
			}
		case KSVAUDIT_STOP:
			{
				notify_message = _("<b>\t已停止录屏</b>\n");
				notify_info(notify_message);
				break;
			}
		case KSVAUDIT_TIMING:
			{
				if (timing)
				{
					QString tmp = QString("%1").arg(timing);
					QString str = _("<b>\t已录屏") + tmp + _("分钟</b>\n");
					notify_info(str.toStdString().c_str());
				}

				break;
			}
		case KSVAUDIT_DISK:
			{
				QString tmp = QString("%1").arg(m_reserveSize);
				QString str = _("<b>\t请先清理磁盘空间至少预留") + tmp + _("G，否则将无法进行录屏!</b>\n");
				notify_warn(str.toStdString().c_str(), 0, "disk_notify");
			}
		default:
			break;
	}

	setreuid(m_source_uid, m_source_uid);
}

KyNotify::~KyNotify()
{
	if (m_bContinue)
	{
		KLOG_INFO() << "clear audit notify data";
		if (m_pDiskNotify)
		{
			KLOG_INFO() << "close notify window";
			notify_notification_close((NotifyNotification *)m_pDiskNotify, NULL);
			g_object_unref(m_pDiskNotify);
			m_pDiskNotify = nullptr;
		}

		notify_uninit();
	}
}

void KyNotify::notify_send(const char *msg, const char *icon, int timeout, const char *userdata)
{
#ifdef HIGH_VERSION
	NotifyNotification *pNotify = notify_notification_new(_("提示"), msg, icon, NULL);
	KLOG_DEBUG() << "not 3.2-8, icon:" << icon;
#else
	NotifyNotification *pNotify = notify_notification_new(_("提示"), msg, icon, NULL);
	KLOG_DEBUG() << "is 3.2-8, icon:" << icon;
#endif
	notify_notification_set_timeout(pNotify, timeout);
	GError *error = NULL;
	if (!notify_notification_show(pNotify, &error))
	{
		KLOG_INFO() << "Error while displaying notification:" << error->message;
		g_error_free(error);
	}

	if (userdata)
	{
		char *tmp = (char *)userdata;
		if (tmp == "disk_notify")
		{
			m_pDiskNotify = (void *)pNotify;
		}
		g_signal_connect(pNotify, "closed", G_CALLBACK(notification_closed), (void *)userdata);
	}
}

void KyNotify::notify_info(const char *msg, int timeout, const char  *userdata)
{
	notify_send(msg, "gtk-dialog-info", timeout, userdata);
}

void KyNotify::notify_warn(const char *msg, int timeout, const char *userdata)
{
	notify_send(msg, "gtk-dialog-warning", timeout, userdata);
}

void KyNotify::notify_error(const char *msg, int timeout, const char  *userdata)
{
	notify_send(msg, "gtk-dialog-error", timeout, userdata);
}
