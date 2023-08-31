#include "audit-notify.h"
#include "record-notify.h"
#include "kiran-log/qt5-log-i.h"
#include <QApplication>
#include <QStringList>
#include <sys/prctl.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

static void sig_handler(int sig)
{
	KLOG_INFO() << "receive signal, cur display:" << getenv("DISPLAY") << "sig:" << sig;
    AuditNotify::instance().clearData();
    qApp->quit();
}

int main(int argc, char **argv)
{
    prctl(PR_SET_PDEATHSIG, SIGINT);
    int ret = klog_qt5_init("", "kylinsec-session", "ks-vaudit", "ks-vaudit-notify");
    if (ret == 0)
        KLOG_INFO() << "init klog succeed";
    else
        qWarning() << "init klog failed";

    signal(SIGINT, sig_handler);
    KLOG_INFO() << "compile time: " << __DATE__ << __TIME__ << "ppid:" << getppid() << "display:" << getenv("DISPLAY") << getenv("XAUTHORITY");
    QApplication a(argc, argv);

    QStringList args = QApplication::arguments();
    KLOG_INFO() << "arguments:" << args << args.count();
    if (args.count() != 2 || (!args[1].startsWith("--record") && !args[1].startsWith("--audit")))
    {
        KLOG_ERROR() << "param err";
        return -1;
    }

    QString arg = args[1];
    QStringList strlist = args[1].split(" ");
	strlist.removeAll("");
    if (strlist.size() != 4)
    {
        KLOG_ERROR() << "param err";
        return -1;
    }

    // 前台提示，只提示一次，2s退出 --record operate uid timing
    if (strlist[0] == ("--record"))
    {
        KLOG_INFO() << "record notify" << strlist;
        RecordNotify::instance().sendNotify(strlist[1], strlist[2].toInt(), strlist[3].toInt());
        return 0;
    }

    // 后台提示，长时间提示，被关闭需要再次提示 --audit disk uid disk-reserve-size
    KLOG_INFO() << "audit notify" << strlist;
    AuditNotify::instance().setParam(strlist[2].toInt(), strlist[3].toLongLong());
    AuditNotify::instance().sendNotify();
    return a.exec();
}
