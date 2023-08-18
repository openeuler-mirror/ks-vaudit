/***************************************************************************
 *                                                                         *
 *   Copyright ©2020 KylinSec. All rights reserved.                        *
 *                                                                         *
 ***************************************************************************/
#include "sql-configure.h"
#include "vaudit-configure.h"
#include "general-configure.h"
#include "configure_adaptor.h"
#include <QDBusConnection>
#include <QDBusError>

#define SIZE_1G (1*1024*1024*1024)

VauditConfigureDbus::VauditConfigureDbus(QObject *parent) : QObject(parent)
{
    new ConfigureAdaptor(this);
    QDBusConnection sysConnection = QDBusConnection::systemBus();
    if (!sysConnection.isConnected())
    {
        qWarning("Cannot connect to the D-Bus system bus.\n"
                 "Please check your system settings and try again.\n");
        exit(1);
    }

    if (!sysConnection.registerObject(KSVAUDIT_CONFIGURE_PATH_NAME, KSVAUDIT_CONFIGURE_INTERFACE_NAME, this))
    {
        qWarning() << "error: register objects failed " << sysConnection.lastError().message();
        exit(2);
    }

    if (!sysConnection.registerService(KSVAUDIT_CONFIGURE_SERVICE_NAME))
    {
        qWarning() << "error: register service failed " << sysConnection.lastError().message();
        exit(3);
    }

    GeneralConfigure::Instance();
    SQLConfigure::Instance();
}


QString VauditConfigureDbus::GetRecordInfo()
{
    return GeneralConfigure::Instance().readRecordConf();
}

QString VauditConfigureDbus::GetAuditInfo()
{
    return GeneralConfigure::Instance().readAuditConf();
}

QString VauditConfigureDbus::GetUserInfo(const QString value)
{
    return SQLConfigure::Instance().queryUser(value);
}

bool VauditConfigureDbus::SetAuditItemValue(const QString &item_value)
{
    if (!GeneralConfigure::Instance().setAuditConfigure(item_value))
        return false;

    this->ConfigureChanged("audit", item_value);
    return true;
}

bool VauditConfigureDbus::SetRecordItemValue(const QString &item_value)
{
    if (!GeneralConfigure::Instance().setRecordConfigure(item_value))
        return false;

    this->ConfigureChanged("record", item_value);
    return true;
}

bool VauditConfigureDbus::CreateUser(const QString info)
{
    return SQLConfigure::Instance().createUser(info);
}

bool VauditConfigureDbus::DeleteUser(const QString info)
{
    return SQLConfigure::Instance().deleteUser(info);
}

bool VauditConfigureDbus::ModifyUserInfo(const QString info)
{
    return SQLConfigure::Instance().updateUser(info);
}

void VauditConfigureDbus::SwitchControl(int from_pid, int to_pid, const QString &operate)
{
    qWarning() << __func__ << from_pid << to_pid << operate;
    this->SignalSwitchControl(from_pid, to_pid, operate);
}
