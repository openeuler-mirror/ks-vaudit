#include "record-notify.h"
#include <glib/gi18n.h>
#include <locale.h>
#include <libnotify/notify.h>
#include "kiran-log/qt5-log-i.h"

#define GETTEXT_DOMAIN "kylin"
#define LICENSE_LOCALEDIR "/usr/share/locale"
#define NOTIFY_TIMEOUT (2 * 1000) // 2 seconds

RecordNotify::RecordNotify()
{
	KLOG_INFO() << "record operate notify init";
	if (!notify_init(_("kylin verify")))
	{
		KLOG_ERROR() << "Failed to initialize libnotify";
		return;
	}
}

RecordNotify& RecordNotify::instance()
{
	static RecordNotify g_rnotify;
	return g_rnotify;
}

void RecordNotify::sendNotify(QString op, uid_t uid, int timing)
{
	QString notify_message;
	if (op == "start")
	{
		notify_message = _("<b>\t已开始录屏</b>\n");
	}
	else if (op == "pause")
	{
		notify_message = _("<b>\t已暂停录屏</b>\n");
	}
	else if (op == "restart")
	{
		notify_message = _("<b>\t已开始录屏</b>\n");
	}
	else if (op == "stop")
	{
		notify_message = _("<b>\t已停止录屏</b>\n");
	}
	else if (op == "timing")
	{
		QString tmp = QString("%1").arg(timing);
		notify_message = _("<b>\t已录屏") + tmp + _("分钟</b>\n");
	}
	else
	{
		KLOG_INFO() << "param err" << op << timing;
		return;
	}

	KLOG_INFO() << "notify info:" << notify_message;
	setreuid(uid, uid);
	notify_send(notify_message.toStdString().c_str());
}

RecordNotify::~RecordNotify()
{
	KLOG_INFO() << "destructor";
	notify_uninit();
}

void RecordNotify::notify_send(const char *msg)
{
	setlocale(LC_ALL, "");
	bindtextdomain(GETTEXT_DOMAIN, LICENSE_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_DOMAIN, "UTF-8");
	textdomain(GETTEXT_DOMAIN);
	const char *icon = "gtk-dialog-info";

#ifdef HIGH_VERSION
	NotifyNotification *pNotify = notify_notification_new(_("提示"), msg, icon);
	KLOG_DEBUG() << "not 3.2-8, icon:" << icon;
#else
	NotifyNotification *pNotify = notify_notification_new(_("提示"), msg, icon, NULL);
	KLOG_DEBUG() << "is 3.2-8, icon:" << icon;
#endif
	notify_notification_set_timeout(pNotify, NOTIFY_TIMEOUT);
	GError *error = NULL;
	if (!notify_notification_show(pNotify, &error))
	{
		KLOG_INFO() << "Error while displaying notification:" << error->message;
		g_error_free(error);
	}

	g_object_unref(pNotify);
}