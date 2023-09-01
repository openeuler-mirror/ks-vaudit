#ifndef DBUSUTILS_H
#define DBUSUTILS_H

#include <QObject>

#define LICENSE_OBJECT_RECORD_NAME "KSVAUDITRECORD"
#define TIMEOUT_MS 5000
#define LICENSE_OBJECT_DBUS_NAME "com.kylinsec.Kiran.LicenseObject"
#define METHOD_GET_LICENSE "GetLicense"
#define METHOD_GET_LICENSE_JSON "GetLicenseJson"
#define METHOD_GET_LICENSE_OBJECK "GetLicenseObject"
#define METHOD_REGISTER_BY_LICENSE_CODE "RegisterByLicenseCode"

enum DbusInterface
{
    ACTIVATE_BYACTIVATIONCODE,
    GET_LICENSE
};

class DBusUtils : public QObject
{
    Q_OBJECT
public:
    DBusUtils(QObject* parent = 0);
    ~DBusUtils();
    QString callGetLicenseInterface();
    bool callActivateInterface(QString args);

signals:
    void standardChanged(uint type);
    void callDbusFailed();
    void LicenseChanged(bool);

private:
    bool creatObjectName();
    QString GetLicense();
    bool ActivateByActivationCode(QString activation_Code, QString &errorMsg);
};

#endif  // DBUSUTILS_H
