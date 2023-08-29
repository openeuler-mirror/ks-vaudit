#ifndef LICENSE_ENTRY_H
#define LICENSE_ENTRY_H

#include "license.h"
#include "dbus-utils.h"
#include <QObject>

class LicenseEntry : public QObject
{
    Q_OBJECT

public:
    explicit LicenseEntry();
    static LicenseEntry &instance();
    void getLicenseInfo(bool &bActive, QString &machineCode, QString &activeCode, QString &expiredTime);
    bool activation(QString activeCode);

private:
    ~LicenseEntry();
    void getLicense(QString license_str);

private slots:
    void onUpdateLicense(bool ret);
    void onErrorDeal();

 private:
    License *m_license;
    DBusUtils *m_dbusutil;
};

#endif // LICENSE_ENTRY_H
