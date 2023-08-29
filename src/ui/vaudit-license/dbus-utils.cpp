#include "dbus-utils.h"
#include <kiran-log/qt5-log-i.h>
#include <kylin-license/license-i.h>
#include <QString>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QtDBus>

DBusUtils::DBusUtils(QObject *parent) : QObject(parent)
{
    creatObjectName();
}

DBusUtils::~DBusUtils()
{
}

bool DBusUtils::creatObjectName()
{
    QDBusMessage msgMethodCall = QDBusMessage::createMethodCall(LICENSE_MANAGER_DBUS_NAME,
                                                                LICENSE_MANAGER_OBJECT_PATH,
                                                                LICENSE_MANAGER_DBUS_NAME,
                                                                METHOD_GET_LICENSE_OBJECK);
    msgMethodCall << LICENSE_OBJECT_RECORD_NAME;

    QDBusMessage msgReply = QDBusConnection::systemBus().call(msgMethodCall, QDBus::Block, TIMEOUT_MS);
    QString errorMsg;
    if (msgReply.type() == QDBusMessage::ReplyMessage)
    {
        QList<QVariant> args = msgReply.arguments();
        if (args.size() >= 1)
            return true;

        errorMsg = "arguments size < 1";
    }

    KLOG_WARNING() << LICENSE_DBUS_NAME << METHOD_GET_LICENSE_JSON
                   << msgReply.errorName() << msgReply.errorMessage() << errorMsg;
    return false;
}

QString DBusUtils::GetLicense()
{
    QDBusMessage msgMethodCall = QDBusMessage::createMethodCall(LICENSE_MANAGER_DBUS_NAME,
                                                                QString(LICENSE_OBJECT_OBJECT_PATH) + "/" + LICENSE_OBJECT_RECORD_NAME,
                                                                LICENSE_OBJECT_DBUS_NAME,
                                                                METHOD_GET_LICENSE);
    QDBusMessage msgReply = QDBusConnection::systemBus().call(msgMethodCall, QDBus::Block, TIMEOUT_MS);
    KLOG_DEBUG() << "msgReply " << msgReply;
    QString errorMsg;

    if (msgReply.type() == QDBusMessage::ReplyMessage)
    {
        QList<QVariant> args = msgReply.arguments();
        if (args.size() >= 1)
        {
            QVariant firstArg = args.takeFirst();
            return firstArg.toString();
        }

        errorMsg = "arguments size < 1";
    }

    KLOG_WARNING() << LICENSE_DBUS_NAME << METHOD_GET_LICENSE_JSON
                   << msgReply.errorName() << msgReply.errorMessage() << errorMsg;
    return "";
}

bool DBusUtils::ActivateByActivationCode(QString activation_Code, QString &errorMsg)
{
    QDBusMessage msgMethodCall = QDBusMessage::createMethodCall(LICENSE_MANAGER_DBUS_NAME,
                                                                QString(LICENSE_OBJECT_OBJECT_PATH) + "/" + LICENSE_OBJECT_RECORD_NAME,
                                                                LICENSE_OBJECT_DBUS_NAME,
                                                                "ActivateByActivationCode");
    msgMethodCall << activation_Code;
    QDBusMessage msgReply = QDBusConnection::systemBus().call(msgMethodCall, QDBus::Block, TIMEOUT_MS);
    KLOG_DEBUG() << "msgReply " << msgReply;

    if (msgReply.type() != QDBusMessage::ErrorMessage)
        return true;

    errorMsg = msgReply.errorMessage();
    KLOG_WARNING() << LICENSE_DBUS_NAME << METHOD_REGISTER_BY_LICENSE_CODE
                   << msgReply.errorName() << msgReply.errorMessage();
    return false;
}

QString DBusUtils::callGetLicenseInterface()
{
    QString ret = GetLicense();
    if (ret == QString("failed"))
        emit callDbusFailed();

    KLOG_DEBUG() << ret;
    return ret;
}

bool DBusUtils::callActivateInterface(QString args)
{
    QString errorMsg{};
    bool ret = ActivateByActivationCode(args, errorMsg);
    emit LicenseChanged(ret);

    KLOG_DEBUG() << ret << errorMsg;
    return ret;
}
