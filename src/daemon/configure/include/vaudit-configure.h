#ifndef VAUDITCONFIGURE_H
#define VAUDITCONFIGURE_H

#include "ksvaudit-configure_global.h"
#include <QObject>

class KSVAUDITCONFIGUREDBUSSHARED_EXPORT VauditConfigureDbus : public QObject
{
    Q_OBJECT
public:
    explicit VauditConfigureDbus(QObject *parent = nullptr);

public slots:
    QString GetRecordInfo();
    QString GetAuditInfo();
    QString GetUserInfo(const QString);
    bool SetAuditItemValue(const QString &item_value);
    bool SetRecordItemValue(const QString &item_value);
    bool CreateUser(const QString);
    bool DeleteUser(const QString);
    bool ModifyUserInfo(const QString);
    void SwitchControl(int from_pid, int to_pid, const QString &operate);
    void MonitorNotification(int pid, const QString &message);

signals:
    void ConfigureChanged(const QString &which, const QString &changed_config);
    void SignalSwitchControl(int from_pid, int to_pid, const QString &operate);
    void SignalNotification(int pid, const QString &message);

private slots:
    void externalConfigureChanged(QString which, QString changed_config);
};

#endif // VAUDITCONFIGURE_H


