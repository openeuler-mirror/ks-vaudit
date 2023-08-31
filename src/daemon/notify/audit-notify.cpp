#include "audit-notify.h"
#include <glib/gi18n.h>
#include <locale.h>
#include <libnotify/notify.h>
#include "kiran-log/qt5-log-i.h"
#include <thread>

#define GETTEXT_DOMAIN "kylin"
#define LICENSE_LOCALEDIR "/usr/share/locale"

static void notification_closed(NotifyNotification *pNotify, gpointer *userdata)
{
	KLOG_INFO() << "close notify window";
	g_object_unref(pNotify);
	if (userdata)
	{
		KLOG_INFO() << "open notify window, userdata:"<< (char *)userdata;
		AuditNotify::instance().sendNotify();
	}
}

void thread_check_ppid(void *user)
{
	AuditNotify *pThis = (AuditNotify *)user;
	while (1)
	{
		sleep(1);
		if (getppid() == 1)
		{
			KLOG_INFO() << "parent process exit";
			pThis->clearData();
		}
	}
}

AuditNotify::AuditNotify() : m_reserveSize(10), m_pNotify(nullptr), m_uid(0)
{
	KLOG_INFO() << "audit disk space notify init";
	setlocale(LC_ALL, "");
	bindtextdomain(GETTEXT_DOMAIN, LICENSE_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_DOMAIN, "UTF-8");
	textdomain(GETTEXT_DOMAIN);

	if (!notify_init(_("kylin verify")))
	{
		KLOG_ERROR() << "Failed to initialize libnotify";
		return;
	}

	std::thread t(&thread_check_ppid, this);
	t.detach();
}

AuditNotify& AuditNotify::instance()
{
	static AuditNotify g_anotify;
	return g_anotify;
}

void AuditNotify::setParam(uid_t uid, quint64 reserveSize)
{
	m_uid = uid;
	m_reserveSize = reserveSize / 1073741824 + 10;
	KLOG_INFO() << "source uid:" << getuid() << "uid:" << m_uid << "reserveSize" << reserveSize;
}

void AuditNotify::sendNotify()
{
	uid_t real_uid;
	uid_t effective_uid;
	uid_t save_uid;
	getresuid(&real_uid, &effective_uid, &save_uid);
	KLOG_INFO("start source id: %d -%d -%d \n", real_uid, effective_uid, save_uid);
	int ret = setreuid(m_uid, m_uid);
	if (ret != 0)
	{
		KLOG_INFO() << "send notify error" << ret;
	}

	char *notify_message = NULL;
	QString tmp = QString("%1").arg(m_reserveSize);
	QString str = _("<b>\t请先清理磁盘空间至少预留") + tmp + _("G，否则将无法进行录屏!</b>\n");
	notify_send(str.toStdString().c_str(), "gtk-dialog-warning", 0, "disk_notify");
}

AuditNotify::~AuditNotify()
{
	KLOG_INFO() << "destructor";
	clearData();
}

void AuditNotify::clearData()
{
	KLOG_INFO() << "clear audit notify data";
	if (m_pNotify)
	{
		KLOG_INFO() << "close notify window";
		notify_notification_close((NotifyNotification *)m_pNotify, NULL);
		g_object_unref(m_pNotify);
		m_pNotify = nullptr;
	}

	notify_uninit();
}

void AuditNotify::notify_send(const char *msg, const char *icon, int timeout, const char *userdata)
{
#ifdef HIGH_VERSION
	NotifyNotification *pNotify = notify_notification_new(_("提示"), msg, icon);
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
		notify_notification_close(pNotify, NULL);
		g_object_unref(pNotify);
		sleep(5);
		notify_uninit();
		exit(1);
		return;
	}

	m_pNotify = (void *)pNotify;
	KLOG_INFO() << m_pNotify << pNotify;
	g_signal_connect(pNotify, "closed", G_CALLBACK(notification_closed), (void *)userdata);
}