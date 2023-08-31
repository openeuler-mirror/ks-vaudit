#include "widget.h"
#include <QApplication>
#include <QFile>
#include <QDesktopWidget>
#include <sys/types.h>
#include <unistd.h>
#include "kiran-log/qt5-log-i.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 加载KLOG
    int ret = klog_qt5_init("", "kylinsec-session", "ks-vaudit", "ks-vaudit-record");
    if (ret != 0){
        KLOG_DEBUG() << "init failed: " << ret;
    }else{
        KLOG_DEBUG() << "succeed";
    }

    Widget w;
    // 加载样式
    QFile file(":/ks-vaudit-record.qss");
    if (file.open(QFile::ReadOnly)){
        QString stylesheet = QLatin1Literal(file.readAll());
        w.setStyleSheet(stylesheet);

        file.close();
    }else{
        KLOG_DEBUG("No qss found!");
    }
    w.move((a.desktop()->width() - w.width()) / 2, (a.desktop()->height() - w.height()) / 2);
    w.show();


    return a.exec();
}
