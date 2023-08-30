#include "monitor.h"
#include "kiran-log/qt5-log-i.h"
#include <QCoreApplication>

int main(int argc, char **argv)
{
    int ret = klog_qt5_init("", "kylinsec-system", "ks-vaudit", "ks-vaudit-monitor");
    if (ret == 0)
        KLOG_INFO() << "init klog succeed";
    else
        qWarning() << "init klog failed";

    QCoreApplication a(argc, argv);
    Monitor monitor;
    return a.exec();
}
