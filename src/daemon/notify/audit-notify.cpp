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
	KLOG_INFO() << "close notify window and re-open";
	g_object_unref(pNotify);
	if (userdata)
	{
		AuditNotify::instance().sendNotify();
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
	int ret = setreuid(m_uid, m_uid);
	if (ret != 0)
	{
		KLOG_INFO() << "send notify error" << ret;
	}

	char *notify_message = NULL;
	QString tmp = QString("%1").arg(m_reserveSize);
	QString str = _("<b>\t请先清理磁盘空间至少预留") + tmp + _("G，否则将无法进行录屏!</b>\n");
	// 超时时间改为10s，解决切换用户发送提示成功，但不显示的问题（重新发送）
	notify_send(str.toStdString().c_str(), "gtk-dialog-warning", 10*1000, "disk_notify");
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
	g_signal_connect(pNotify, "closed", G_CALLBACK(notification_closed), (void *)userdata);
}
