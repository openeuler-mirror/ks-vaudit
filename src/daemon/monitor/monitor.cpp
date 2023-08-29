#include "kiran-log/qt5-log-i.h"
#include "monitor.h"
#include "monitor-disk.h"
#include <kylin-license/license-i.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <QTimer>
#include <QDir>
#include <QMutexLocker>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QtDBus>

//dbus-send --system  --print-reply --type=method_call --dest=org.gnome.DisplayManager /org/gnome/DisplayManager/Manager org.gnome.DisplayManager.Manager.GetDisplays
//dbus-send --system --print-reply --type=method_call --dest=org.gnome.DisplayManager /org/gnome/DisplayManager/Display2 org.gnome.DisplayManager.Display.GetX11DisplayName
//张三_192.168.1.1_20220621_153620.mp4

#define TIMEOUT_MS 5000

Monitor::Monitor(QObject *parent)
{
    m_isActive = isLicenseActive();
    KLOG_DEBUG() << "Current thread ID: " << QThread::currentThreadId() << "m_isActive:" << m_isActive;
    QDir dir;
    m_filePath = "/var/log/kylinsec/ks-vaudit/monitor/";
    dir.mkpath(m_filePath);
    m_vauditBin = "/usr/bin/ks-vaudit";
    m_pTimer = new QTimer();
    connect(m_pTimer, SIGNAL(timeout()), this, SLOT(monitorProcess()));
    connect(&MonitorDisk::instance(), SIGNAL(SignalNotification(int, QString)), this, SLOT(receiveNotification(int, QString)));
    monitorProcess();
}

Monitor::~Monitor()
{
    if (m_pTimer)
        delete m_pTimer;
    m_pTimer = nullptr;

    for (auto it = m_sessionInfos.begin(); it != m_sessionInfos.end();)
    {
        auto &process = it.value().process;
        if (process)
        {
            process->execute("taskkill", QStringList() << "-PID" << QString("%1").arg(process->processId()) << "-F" <<"-T");
            process->close();
            delete process;
            process = nullptr;
        }
        m_sessionInfos.erase(it++);
    }
}

void Monitor::monitorProcess()
{
    if (m_pTimer->isActive())
        m_pTimer->stop();

    if (m_isActive)
    {
        QFile file(m_vauditBin);
        if (file.exists())
        {
            DealSession();
            MonitorDisk::instance().fileDiskLimitProcess();
            MonitorDisk::instance().fileSizeProcess(m_videoFileName);
        }
    }
    else
    {
        m_isActive = isLicenseActive();
    }

    m_pTimer->start(2000);
}

void Monitor::receiveNotification(int pid, QString message)
{
    KLOG_DEBUG() << pid << message << "m_sessionInfos size:" << m_sessionInfos.size() << "m_videoFileName size:" <<  m_videoFileName.size();

    for (auto it = m_sessionInfos.begin(); it != m_sessionInfos.end(); ++it)
    {
        auto &process = it.value().process;
        if (process && process->pid() == pid)
        {
            QFileInfo fileinfo(message);
            const QString &suffix = fileinfo.suffix();
            if (suffix == "mp4" || suffix == "ogv" || suffix == "MP4" || suffix == "OGV")
                m_videoFileName.insert(pid, message);

            return;
        }
    }

}

QMap<QString, QString> Monitor::getXorgLoginName()
{
    QMap<QString, QString> map;
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.setProgram("sh");
    process.setArguments(QStringList() << "-c" << "who -u | grep tty");
    process.start();
    if (process.waitForFinished())
    {
        QByteArray data = process.readAll();
        QString str(data.toStdString().data());
        QStringList strlist = str.split("\n");
        strlist.removeAll("");
        for (QString v : strlist)
        {
            QStringList arr = v.split(" ");
            arr.removeAll("");
            int index = arr.indexOf(QRegularExpression("^tty[0-9]$"));
            if (index == -1)
                continue;
            map.insert(arr[index], arr[0]);
        }
    }

    return map;
}

QStringList Monitor::getVncProcessId()
{
    QStringList strlist;
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.setProgram("sh");
    process.setArguments(QStringList() << "-c" << "ps aux | grep Xvnc | grep -v grep | awk '{print $2}'");
    process.start();
    if (process.waitForFinished())
    {
        QByteArray data = process.readAll();
        QString str(data.toStdString().data());
        strlist = str.split("\n");
        strlist.removeAll("");
    }

    return strlist;
}

QVector<sessionInfo> Monitor::getXorgInfo()
{
    QVector<sessionInfo> vecInfo;
    QMap<QString, QString> map = getXorgLoginName();
    if (map.isEmpty())
        return vecInfo;

    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.setProgram("sh");
    process.setArguments(QStringList() << "-c" << "ps aux | grep Xorg | grep -v grep");
    process.start();
    if (process.waitForFinished())
    {
        QByteArray data = process.readAll();
        QString str(data.toStdString().data());
        QStringList strlist = str.split("\n");
        strlist.removeAll("");
        //KLOG_DEBUG() << "Xorg info:" << strlist << strlist.size();
        for (QString v : strlist)
        {
            QStringList arr = v.split(" ");
            arr.removeAll("");
            int index  = arr.indexOf(QRegularExpression("^tty[0-9]$"));
            int index1 = arr.indexOf("/usr/bin/Xorg");
            int index2 = arr.indexOf(QRegularExpression("^-auth"));
            if (index == -1 || index1 == -1 || index2 == -1)
                continue;

            QString ttyName = arr[index];
            auto it = map.find(ttyName);
            if (it == map.end())
                continue;

            struct sessionInfo info = {it.value(), "127.0.0.1", arr[index1+1], arr[index2 + 1], nullptr};
            vecInfo.push_back(info);
        }
    }

    return vecInfo;
}

QString Monitor::getRemotIP(const QString &pid)
{
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.setProgram("sh");
    QString arg = "netstat -antp | grep " + pid + " | grep ESTABLISHED | awk '{print $5}'";
    process.setArguments(QStringList() << "-c" << arg);
    process.start();
    if (process.waitForFinished())
    {
        QByteArray data = process.readAll();
        QString str(data.toStdString().data());
        QStringList strlist = str.split("\n");
        strlist.removeAll("");
        KLOG_DEBUG() << "pid:" << pid << "strlist:" << strlist;
        for (QString v : strlist)
        {
            QStringList arr = v.split(":");
            if (2 == arr.size())
                return arr[0];
        }
    }

    return "";
}

QVector<sessionInfo> Monitor::getXvncInfo()
{
    QVector<sessionInfo> vecInfo;
    QStringList pids = getVncProcessId();
    for (auto pid : pids)
    {
        QString ip = getRemotIP(pid);
        if (ip.isEmpty())
            continue;

        QProcess process;
        process.setProcessChannelMode(QProcess::MergedChannels);
        QString arg = "cat /proc/" + pid + "/cmdline";
        process.start(arg);
        if (process.waitForFinished())
        {
            QByteArray data = process.readAll();

            QString str;
            QStringList strlist;
            for (int i = 0; i < data.size(); i++)
            {
                if (data[i] == '\x00' || data[i] == ' ')
                {
                    strlist.append(str);
                    str.clear();
                    continue;
                }
                str = str + data[i];
            }

            int index = strlist.indexOf("/usr/bin/Xvnc");
            int index1 = strlist.indexOf(QRegularExpression("^-auth"));
            int index2 = strlist.indexOf(QRegExp("\\(.*\\)"));
            if (index == -1 || index1 == -1 || index2 == -1)
                continue;

            QString tmp = strlist[index2];
            struct sessionInfo info = {tmp.mid(1, tmp.size() - 2), ip, strlist[index+1], strlist[index1+1], nullptr};
            vecInfo.push_back(info);
        }
    }

    return vecInfo;
}

QProcess* Monitor::startRecordWithDisplay(sessionInfo info)
{
    QProcess *process = new QProcess();
    process->setProcessChannelMode(QProcess::MergedChannels);
    QString logFile = m_filePath + "display" + info.displayName;
    process->setStandardOutputFile(logFile + QString(".out"));
    process->setStandardErrorFile(logFile + QString(".err"));
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("DISPLAY", info.displayName); // Add an environment variable
    env.insert("XAUTHORITY", info.authFile);
    process->setProcessEnvironment(env);
    QStringList arg;
    arg << (QString("--audit-") + info.userName + QString("-") + info.ip + QString("-") + info.displayName);

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [=](int exitCode, QProcess::ExitStatus exitStatus) {
        KLOG_WARNING() << "pid:" << process->pid() << "arg:" << process->arguments()  << "exit, exitcode:" << exitCode << exitStatus;
        sessionInfo tmp = info;
        QMutexLocker locker(&m_mutex);
        auto it = m_sessionInfos.find(tmp.userName, tmp);
        if (it != m_sessionInfos.end())
        {
            auto &value = it.value();
            KLOG_INFO() << "restart bin, find" << it.key() << value.displayName;
            auto &process = value.process;
            if (process)
            {
                auto fileit = m_videoFileName.find(process->pid());
                if (fileit != m_videoFileName.end())
                    m_videoFileName.erase(fileit);

                process->execute("taskkill", QStringList() << "-PID" << QString("%1").arg(process->processId()) << "-F" <<"-T");
                process->close();
                delete process;
                process = nullptr;
            }
            m_sessionInfos.erase(it);
            if (QProcess::ExitStatus::CrashExit == exitStatus)
            {
                tmp.process = startRecordWithDisplay(tmp);
                m_sessionInfos.insert(tmp.userName, tmp);
            }
        }
    });

    process->start(m_vauditBin, arg);

    KLOG_DEBUG() << "env display:" << info.displayName << info.authFile << process->pid() << process->program() << process->arguments();
    return process;
}

void Monitor::DealSession()
{
    int maxRecordPerUser = MonitorDisk::instance().getMaxRecordPerUser();
    QVector<sessionInfo> infos = getXorgInfo();
    QVector<sessionInfo> xvncInfos = getXvncInfo();
    infos.append(xvncInfos);

    QMutexLocker locker(&m_mutex);
    QMap<QString, int>mapCnt;
    for (auto it = m_sessionInfos.begin(); it != m_sessionInfos.end(); ++it)
    {
        mapCnt.insert(it.key(), ++mapCnt[it.key()]);
    }

    QMultiMap<QString, sessionInfo> sessionInfos;
    for (sessionInfo info : infos)
    {
        sessionInfos.insert(info.userName, info);
    }
    for (auto it = m_sessionInfos.begin(); it != m_sessionInfos.end();)
    {
        auto &info = it.value();
        if (!sessionInfos.contains(it.key(), info))
        {
            auto &process = info.process;
            if (process)
            {
                process->execute("taskkill", QStringList() << "-PID" << QString("%1").arg(process->processId()) << "-F" <<"-T");
                process->close();
                delete process;
                process = nullptr;
            }
            KLOG_DEBUG() << "not contain and erase:" << it.key() << info.displayName;
            m_sessionInfos.erase(it++);
            continue;
        }

        ++it;
    }

    for (sessionInfo info : infos)
    {
        if (!m_sessionInfos.contains(info.userName, info))
        {
            if (maxRecordPerUser > 0 && mapCnt[info.userName] >= maxRecordPerUser)
            {
                KLOG_INFO() << info.userName << "max limit reached:" << mapCnt[info.userName] << ">" << maxRecordPerUser;
                continue;
            }
            mapCnt.insert(info.userName, ++mapCnt[info.userName]);
            info.process = startRecordWithDisplay(info);
            m_sessionInfos.insert(info.userName, info);
        }
    }
}

bool Monitor::isLicenseActive()
{
    QDBusMessage msgMethodCall = QDBusMessage::createMethodCall(LICENSE_MANAGER_DBUS_NAME,
                                                                QString(LICENSE_OBJECT_OBJECT_PATH) + "/KSVAUDITRECORD",
                                                                 "com.kylinsec.Kiran.LicenseObject",
                                                                "GetLicense");
    QDBusMessage msgReply = QDBusConnection::systemBus().call(msgMethodCall, QDBus::Block, TIMEOUT_MS);
    KLOG_DEBUG() << "msgReply " << msgReply;
    QString errorMsg;

    if (msgReply.type() == QDBusMessage::ReplyMessage)
    {
        QList<QVariant> args = msgReply.arguments();
        if (args.size() >= 1)
        {
            QVariant firstArg = args.takeFirst();
            KLOG_WARNING() << firstArg.toString();
            QJsonParseError jsonerror;
            QJsonDocument doc = QJsonDocument::fromJson(firstArg.toString().toLatin1(), &jsonerror);
            if (doc.isNull() || jsonerror.error != QJsonParseError::NoError || !doc.isObject())
            {
                KLOG_INFO() << "parse json err";
                return false;
            }

            QJsonObject jsonObj = doc.object();
            for (auto key : jsonObj.keys())
            {
                if ("activation_status" == key)
                {
                    int activation_status = jsonObj[key].toDouble();
                    KLOG_INFO() << "activation_status:" << activation_status;
                    return activation_status != LicenseActivationStatus::LAS_ACTIVATED ? false : true;
                }
            }

            return false;
        }
    }

    return false;
}
