#include "login.h"
#include "widget.h"
#include <QApplication>
#include <QFile>
#include <QDesktopWidget>
#include <QTranslator>
#include "kiran-log/qt5-log-i.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 加载KLOG
    int ret = klog_qt5_init("", "kylinsec-session", "ks-vaudit", "ks-vaudit-audit");
    if (ret != 0){
        KLOG_DEBUG() << "init failed: " << ret;
    }else{
        KLOG_DEBUG() << "succeed";
    }

    // 加载翻译
    // RECORD_SYSTEM_DIR为编译选项
    QString systemTranslationsPath = QString("%1/%2").arg(RECORD_SYSTEM_DIR).arg("translations");
    QTranslator translator_audit;
    if (translator_audit.load(QLocale::system(), "ks-vaudit-audit", ".", QCoreApplication::applicationDirPath(), ".qm")){
        KLOG_DEBUG() << "Loaded compiled translations";
        QApplication::installTranslator(&translator_audit);
    }else if (translator_audit.load(QLocale::system(), "ks-vaudit-audit", ".", systemTranslationsPath)){
        KLOG_DEBUG() << "Loaded installed translations";
        QApplication::installTranslator(&translator_audit);
    }else{
        KLOG_ERROR() << "Translation load failed";
    }

    Login n;// = new Login();
    Widget w;// = new Widget();
    QWidget::connect(&n,SIGNAL(show_widget(QJsonObject)),&w,SLOT(show_widget_page(QJsonObject)));
    QWidget::connect(&w,SIGNAL(log_out()),&n,SLOT(show_logout()));
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
    w.move((a.desktop()->width() - w.width()) / 2, (a.desktop()->height() - w.height()) / 2);

    n.show();

    return a.exec();
}
