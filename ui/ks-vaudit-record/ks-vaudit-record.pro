#-------------------------------------------------
#
# Project created by QtCreator 2022-06-27T06:00:51
#
#-------------------------------------------------

QT       += core gui dbus

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ks-vaudit-record
TEMPLATE = app
SUBDIRS = src \
          data

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
INCLUDEPATH += /usr/local/include /usr/local/include/libavformat
LIBS += -L/usr/local/lib -lavformat -lavcodec -lavutil

SOURCES += main.cpp\
        widget.cpp \
        dialog.cpp

HEADERS  += widget.h \
    dialog.h\
    ksvaudit-configure_global.h

FORMS    += widget.ui \
    dialog.ui

RESOURCES += \
    src.qrc

FORMS +=
DBUS_INTERFACES += com.kylinsec.Kiran.VauditConfigure.xml

DISTFILES += \
    src/a.sh
