#include "widget.h"
#include <QApplication>
#include <QFile>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Widget w;
    w.show();

    // 加载样式
    QFile file(":/ks-vaudit-record.qss");
    if (file.open(QFile::ReadOnly)){
        QString stylesheet = QLatin1Literal(file.readAll());
        a.setStyleSheet(stylesheet);

        file.close();
    }else{
        KLOG_DEBUG("No qss found!");
    }
    return a.exec();
}
