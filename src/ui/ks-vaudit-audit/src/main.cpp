#include "login.h"
#include "widget.h"
#include <QApplication>
#include <QFile>
#include "kiran-log/qt5-log-i.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 加载KLOG
    int ret = klog_qt5_init("", "kylinsec-session", "ks-vaudit", "ks-vaudit-audit");
    if (ret != 0){
        qDebug() << "init failed: " << ret;
    }else{
        KLOG_DEBUG() << "succeed";
    }
    Login n;// = new Login();
    Widget w;// = new Widget();
    QWidget::connect(&n,SIGNAL(show_widget()),&w,SLOT(show_widget_page()));
    // 加载样式
    QFile file(":/ks-vaudit-audit.qss");
    if (file.open(QFile::ReadOnly)){
        QString stylesheet = QLatin1Literal(file.readAll());
        // 需要给两个窗体分别设样式，不然会有随机性的显示不全
        n.setStyleSheet(stylesheet);
        w.setStyleSheet(stylesheet);

        file.close();
    }else{
        KLOG_DEBUG("No qss found!");
    }

    n.show();

    return a.exec();
}
