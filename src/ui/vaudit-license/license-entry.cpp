#include "license-entry.h"
#include <kylin-license/license-i.h>
#include <kiran-log/qt5-log-i.h>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonParseError>

LicenseEntry::LicenseEntry()
{
    m_license = new License;
    m_dbusutil = new DBusUtils(this);

    connect(m_dbusutil, SIGNAL(LicenseChanged(bool)), this, SLOT(onUpdateLicense(bool)));
    connect(m_dbusutil, SIGNAL(callDbusFailed()), this, SLOT(onErrorDeal()));
}

LicenseEntry &LicenseEntry::instance()
{
    static LicenseEntry s_license;
    return s_license;
}

void LicenseEntry::getLicenseInfo(bool &bActive, QString &machineCode, QString &activeCode, QString &expiredTime)
{
    QString licence_str = m_dbusutil->callGetLicenseInterface();
    getLicense(licence_str);

    bActive = m_license->activation_status != LicenseActivationStatus::LAS_ACTIVATED ? false : true;
    machineCode = m_license->machine_code;
    activeCode = m_license->activation_code;
    expiredTime = QDateTime::fromTime_t(m_license->expired_time).toString("yyyy-MM-dd");

    KLOG_DEBUG() << "bActive:" << bActive << "machineCode:" << machineCode << "activeCode:" << activeCode << "expiredTime:" << expiredTime;
}

bool LicenseEntry::activation(QString activeCode)
{
    if (activeCode.isNull())
    {
        KLOG_DEBUG() << "Null activation_code";
        return false;
    }

    m_license->activation_code = activeCode;
    if (!m_dbusutil->callActivateInterface(m_license->activation_code))
    {
        KLOG_ERROR() << "callActivateInterface err";
        return false;
    }

    return true;
}

LicenseEntry::~LicenseEntry()
{
    if (m_dbusutil)
    {
        delete m_dbusutil;
        m_dbusutil = nullptr;
    }

    if (m_license)
    {
        delete m_license;
        m_license = nullptr;
    }
}

void LicenseEntry::getLicense(QString license_str)
{
    QJsonParseError jsonerror;
    QJsonDocument doc = QJsonDocument::fromJson(license_str.toLatin1(), &jsonerror);
    if (!doc.isNull() && jsonerror.error == QJsonParseError::NoError)
    {
        if (doc.isObject())
        {
            QJsonObject object = doc.object();
            QJsonObject::iterator it = object.begin();
            while (it != object.end())
            {
                switch (it.value().type())
                {
                    case QJsonValue::String:
                    {
                        QString jsonKey = it.key();
                        QString jsonString = it.value().toString();
                        if (LICENSE_JK_ACTIVATION_CODE == jsonKey)
                        {
                            m_license->activation_code = jsonString;
                        }
                        else if (LICENSE_JK_MACHINE_CODE == jsonKey)
                        {
                            m_license->machine_code = jsonString;
                        }
                        break;
                    }
                    case QJsonValue::Double:
                    {
                        QString jsonKey = it.key();
                        if (LICENSE_JK_ACTIVATION_STATUS == jsonKey)
                        {
                            m_license->activation_status = it.value().toDouble();
                        }
                        else if (LICENSE_JK_ACTIVATION_TIME == jsonKey)
                        {
                            m_license->activation_time = it.value().toDouble();
                        }
                        else if (LICENSE_JK_EXPIRED_TIME == jsonKey)
                        {
                            m_license->expired_time = it.value().toDouble();
                        }
                        break;
                    }
                    default:
                    {
                        break;
                    }
                }
                it++;
            }
        }
    }
}

void LicenseEntry::onUpdateLicense(bool ret)
{
    if (ret)
    {
        KLOG_DEBUG() << "license changed!";
        QString license_str = m_dbusutil->callGetLicenseInterface();
        getLicense(license_str);

        if (m_license->activation_status == LicenseActivationStatus::LAS_ACTIVATED)
            KLOG_INFO() << tr("Activated");
        KLOG_INFO() << m_license->machine_code << m_license->activation_code << m_license->activation_time << m_license->expired_time;
    }
}

void LicenseEntry::onErrorDeal()
{
    KLOG_DEBUG() << "callDbusFailed";
}

