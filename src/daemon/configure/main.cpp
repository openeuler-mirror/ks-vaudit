#include "vaudit-configure.h"
#include "kiran-log/qt5-log-i.h"
#include <QCoreApplication>

int main(int argc, char *argv[])
{
    int ret = klog_qt5_init("", "kylinsec-session", "ks-vaudit", "ks-vaudit-configure");
    if (ret == 0)
        KLOG_INFO() << "init klog succeed";
    else
        qWarning() << "init klog failed";

    QCoreApplication a(argc, argv);

    VauditConfigureDbus bus;
    return a.exec();
}
