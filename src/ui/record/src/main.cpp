#include "widget.h"
#include <QApplication>
#include <QFile>
#include <QDesktopWidget>
#include <QTranslator>
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

    // 加载翻译
    QString systemTranslationsPath = QString("%1/%2").arg(RECORD_SYSTEM_DIR).arg("translations");
    QTranslator translator_record;
    if (translator_record.load(QLocale::system(), "ks-vaudit-record", ".", QCoreApplication::applicationDirPath(), ".qm")){
        KLOG_DEBUG() << "Loaded compiled translations";
        QApplication::installTranslator(&translator_record);
    }else if (translator_record.load(QLocale::system(), "ks-vaudit-record", ".", systemTranslationsPath)){
        KLOG_DEBUG() << "Loaded installed translations";
        QApplication::installTranslator(&translator_record);
    } else {
        KLOG_INFO() << "load translations failed";
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

    // 设置移动位置参数，在widget里将界面移到屏幕中间
    w.setToCenter((a.desktop()->width() - w.width()) / 2, (a.desktop()->height() - w.height()) / 2);

    return a.exec();
}
