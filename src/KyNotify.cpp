#include "KyNotify.h"
#include <glib.h>
#include <glib/gi18n.h>
#include <locale.h>
#include <libnotify/notify.h>
#include <QString>

#define NOTIFY_TIMEOUT (2 * 1000) // 2 seconds
#define GETTEXT_DOMAIN "kylin"
#define LICENSE_LOCALEDIR "/usr/share/locale"

KyNotify::KyNotify()
{
	initNotify();
	if (!notify_init(_("kylin verify")))
	{
		g_printerr(_("Failed to initialize libnotify\n"));
		return;
	}
}

KyNotify& KyNotify::instance()
{
    static KyNotify g_notify;
	return g_notify;
}

void KyNotify::notify(NOTYFY_MESSAGE msg, int timing)
{
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
		default:
			break;
	}
}

KyNotify::~KyNotify()
{
	notify_uninit();
}

void KyNotify::initNotify()
{
	uid_t user_id = getuid();
	gid_t user_gid = getgid();
	uid_t euid_save = geteuid();
	gid_t egid_save = getegid();
	seteuid(user_id);
	setegid(user_gid);
	setreuid(user_id, user_id);
	setregid(user_gid, user_gid);
}

void KyNotify::notify_send(const char *msg, const char *icon)
{
	NotifyNotification *notify = notify_notification_new(_("提示"), msg, icon, NULL);
	notify_notification_set_timeout(notify, NOTIFY_TIMEOUT);
	notify_notification_show(notify, NULL);
	g_object_unref(G_OBJECT(notify));
}

void KyNotify::notify_info(const char *msg)
{
	notify_send(msg, "gtk-dialog-info");
}

void KyNotify::notify_warn(const char *msg)
{
	notify_send(msg, "gtk-dialog-warning");
}

void KyNotify::notify_error(const char *msg)
{
	notify_send(msg, "gtk-dialog-error");
}

