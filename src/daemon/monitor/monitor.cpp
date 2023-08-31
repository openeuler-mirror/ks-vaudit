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
#include <iostream>
#include <thread>

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
    m_lastMaxRecordPerUser = MonitorDisk::instance().getMaxRecordPerUser();
    monitorProcess();
    m_pFontTimer = new QTimer();
    connect(m_pFontTimer, SIGNAL(timeout()), this, SLOT(recordProcess()));
    connect(&MonitorDisk::instance(), &MonitorDisk::SignalFrontBackend, this, &Monitor::receiveFrontBackend);
    recordProcess();
}

Monitor::~Monitor()
{
    if (m_pTimer)
        delete m_pTimer;
    m_pTimer = nullptr;

    if (m_pFontTimer)
        delete m_pFontTimer;
    m_pFontTimer = nullptr;
    clearSessionInfos();
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
            bool bRet = MonitorDisk::instance().fileDiskLimitProcess();
            DealSession(bRet); //启停进程
            MonitorDisk::instance().fileSizeProcess(m_videoFileName); //文件达到指定大小，切换文件
            MonitorDisk::instance().fixVidoes(); // 修改视频后缀
        }
    }
    else
    {
        m_isActive = isLicenseActive();
    }

    m_pTimer->start(2000);
}

void Monitor::recordProcess()
{
    if (m_pFontTimer->isActive())
        m_pFontTimer->stop();

    // 查询前台进程是否还存在，不存在关闭后台录屏进程，清除信息
    for (auto it = m_frontRecordInfo.begin(); it != m_frontRecordInfo.end();)
    {
        QProcess process;
        process.setProcessChannelMode(QProcess::MergedChannels);
        process.setProgram("sh");
        QString arg = "kill -0 " + QString::number(it.key()) + " >/dev/null 2>&1; echo $?";
        process.setArguments(QStringList() << "-c" << arg);
        process.start();
        if (process.waitForFinished())
        {
            QByteArray data = process.readAll();
            QString str(data.toStdString().data());
            QStringList strlist = str.split("\n");
            strlist.removeAll("");
            if (strlist.size() != 1)
            {
                ++it;
                continue;
            }

            if (strlist[0].toInt() != 0)
            {
                KLOG_INFO() << "front pid:" << it.key() << "is not exist";
                auto &process = it.value();
                m_frontRecordInfo.erase(it++);
                clearProcess(process);
                continue;
            }
        }
        ++it;
    }

    m_pFontTimer->start(2000);
}

void Monitor::receiveNotification(int pid, QString message)
{
    KLOG_INFO() << pid << message << "m_sessionInfos size:" << m_sessionInfos.size() << "m_videoFileName size:" <<  m_videoFileName.size();
    if (message.size() < 8)
        return;

    // 查找是否为后台录屏进程的文件名，并记录后台录屏进程的pid
    bool bInsert{};
    QMap<int, int>map;
    for (auto it = m_sessionInfos.begin(); it != m_sessionInfos.end(); ++it)
    {
        auto &process = it.value().process;
        if (process)
        {
            map.insert(process->pid(), 0);
            if (process->pid() == pid)
            {
                if (message == "disk_notify")
                {
                    it.value().bNotify = true;
                    KLOG_INFO() << "audit receive disk space notify";
                    return;
                }
                QString suffix = message.mid(message.size() - 8, message.size());
                if (suffix == ".mp4.tmp" || suffix == ".ogv.tmp" || suffix == ".MP4.tmp" || suffix == ".OGV.tmp"
                    || suffix == ".mkv.tmp" || suffix == ".MKV.tmp")
                {
                    bInsert = true;
                    it.value().bStart = true;
                }
            }
        }
    }

    // 清掉不存在的进程文件信息，进程挂掉后信号内的进程id为0
    for (auto it = m_videoFileName.begin(); it != m_videoFileName.end();)
    {
        auto iter = map.find(it.key());
        if (iter == map.end())
        {
            KLOG_INFO() << "m_videoFileName erase" << it.key() << it.value();
            m_videoFileName.erase(it++);
            continue;
        }

        ++it;
    }

    if (bInsert)
    {
        m_videoFileName.insert(pid, message);
    }

    KLOG_INFO() << "is insert" << bInsert;
}

void Monitor::receiveFrontBackend(int from_pid, QString displayName, QString authFile, QString userName, QString homeDir)
{
    // 根据前台发送的信息，启动后台进程
    KLOG_INFO() << "from_pid:" << from_pid << "displayName:" << displayName << "authFile:" << authFile << "userName:" << userName << "homeDir:" << homeDir;
    QProcess *process = new QProcess();
    process->setProcessChannelMode(QProcess::MergedChannels);
    QString logFile = m_filePath + "record-display" + displayName;
    process->setStandardOutputFile(logFile + QString(".out"));
    process->setStandardErrorFile(logFile + QString(".err"));
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("DISPLAY", displayName); // Add an environment variable
    env.insert("XAUTHORITY", authFile);
    process->setProcessEnvironment(env);
    QStringList arg;
    arg << QString("--record ") + QString::number(from_pid) + QString(" ") + userName + QString(" ") + homeDir;
    process->start(m_vauditBin, arg);
    KLOG_INFO() << "backend pid:" << process->pid() << process->arguments();
    MonitorDisk::instance().sendProcessPid(process->pid(), from_pid);
    MonitorDisk::instance().sendProcessPid(from_pid, process->pid());
    m_frontRecordInfo.insert(from_pid, process);
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
    process.setArguments(QStringList() << "-c" << "ps aux | grep Xvnc | grep -v grep | awk -v FS=' ' '{print $1,$2}'");
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

            struct sessionInfo info = {it.value(), "127.0.0.1", arr[index1+1], arr[index2 + 1], nullptr, false, false, time(NULL)};
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
    for (auto v : pids)
    {
        QStringList lists = v.split(" ");
        if (lists.size() != 2)
            continue;

        QString user = lists[0];
        QString pid = lists[1];
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

            int index = strlist.indexOf(QRegularExpression(".*Xvnc$"));
            int index1 = strlist.indexOf(QRegularExpression("^-rfbauth"));
            if (index == -1 || index1 == -1)
                continue;

            struct sessionInfo info = {user, ip, strlist[index+1], strlist[index1+1], nullptr, false, false, time(NULL)};
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
    arg << (QString("--audit ") + info.userName + QString(" ") + info.ip + QString(" ") + info.displayName);

    connect(process, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this, [=](int exitCode, QProcess::ExitStatus exitStatus) {
        KLOG_WARNING() << "receive finshed signal: pid:" << process->pid() << "arg:" << process->arguments()  << "exit, exitcode:" << exitCode << exitStatus;
	    QSharedPointer<sessionInfo> ptrTmp(new sessionInfo(info));

        sleep(1);
        sessionInfo &tmp = *ptrTmp.data();
        auto it = m_sessionInfos.find(tmp.userName, tmp);
        if (it != m_sessionInfos.end())
        {
            auto &value = it.value();
            // 找到的不是同一个进程
            if (value.stTime != tmp.stTime)
            {
                KLOG_INFO() << "not find, killed process start time" << tmp.stTime << "find time" << tmp.stTime;
                return;
            }

            KLOG_INFO() << "restart bin, find" << it.key() << value.ip << value.displayName << value.authFile << value.stTime;
            auto &process = value.process;
            if (process)
            {
                process->close();
                delete process;
                process = nullptr;
            }
            value.process = startRecordWithDisplay(tmp);
            value.bStart = false;
            KLOG_INFO() << "deal CrashExit end";
        }
    });

    process->start(m_vauditBin, arg);

    KLOG_INFO() << "env display:" << info.displayName << info.authFile << process->pid() << process->program() << process->arguments();
    return process;
}

void Monitor::DealSession(bool isDiskOk)
{
    // 最大会话数改变处理：关闭之前的进程，并清理会话信息
    int maxRecordPerUser = MonitorDisk::instance().getMaxRecordPerUser();
    if (m_lastMaxRecordPerUser != maxRecordPerUser)
    {
        KLOG_INFO() << "maxRecordPerUser changed, restart all record exec" << maxRecordPerUser << m_lastMaxRecordPerUser;
        m_videoFileName.clear();
        clearSessionInfos();
        m_lastMaxRecordPerUser = maxRecordPerUser;
    }

    // 获取所有图形会话信息(本地和vnc)
    QVector<sessionInfo> infos = getXorgInfo();
    QVector<sessionInfo> xvncInfos = getXvncInfo();
    infos.append(xvncInfos);

    // 将图形信息按用户名为key，保存为map，便于统计一个用户下有多个会话
    QMultiMap<QString, sessionInfo> sessionInfos;
    for (sessionInfo info : infos)
    {
        sessionInfos.insert(info.userName, info);
    }

    // 清除关闭的会话录屏信息并统计单个用户有多少个会话
    QMap<QString, int>mapCnt;
    for (auto it = m_sessionInfos.begin(); it != m_sessionInfos.end();)
    {
        auto &info = it.value();
        if (!sessionInfos.contains(it.key(), info))
        {
            KLOG_INFO() << "not contain and erase:" << it.key() << info.ip << info.displayName;
            auto process = it.value().process;
            m_sessionInfos.erase(it++);
            clearProcess(process);
            continue;
        }

        // 统计单个用户有多少个会话
        mapCnt.insert(it.key(), ++mapCnt[it.key()]);
        ++it;
    }

    // 拉起新启的会话的录屏进程
    for (sessionInfo info : infos)
    {
        if (!m_sessionInfos.contains(info.userName, info))
        {
            // 单个用户最大录屏会话数处理；0 为不限制
            if (maxRecordPerUser > 0 && mapCnt[info.userName] >= maxRecordPerUser)
            {
                //KLOG_INFO() << info.userName << "max limit reached:" << mapCnt[info.userName] << ">" << maxRecordPerUser;
                continue;
            }

            mapCnt.insert(info.userName, ++mapCnt[info.userName]);
            info.process = startRecordWithDisplay(info);
            m_sessionInfos.insert(info.userName, info);
        }
    }

    for (auto it = m_sessionInfos.begin(); it != m_sessionInfos.end(); ++it)
    {
        auto &val = it.value();
        // 磁盘空间正常
        if (isDiskOk)
        {
            // 进程存在，但是没有录屏(以收到录屏文件为准)，发送开始录屏信息
            if (!val.bStart)
            {
                KLOG_INFO() << "record not start and start, pid:" << val.process->processId() << "display:" << val.displayName << "ip:" << val.ip;
                MonitorDisk::instance().sendSwitchControl(val.process->processId(), "start");
            }

            // 关闭提示磁盘空间不足
            if (val.bNotify)
            {
                KLOG_INFO() << "disk space returned to normal, stop notify, pid:" << val.process->processId();
                MonitorDisk::instance().sendSwitchControl(val.process->processId(), "disk_notify_stop");
                val.bNotify = false;
            }
        }
        else
        {
            //磁盘空间不足处理，如果停止过录像并正在提示不处理
            if (!val.bNotify)
            {
                KLOG_INFO() << "insufficient disk space, stop record and notify pid:" << val.process->processId();
                MonitorDisk::instance().sendSwitchControl(val.process->processId(), "stop");
                MonitorDisk::instance().sendSwitchControl(val.process->processId(), "disk_notify");
                val.bStart = false;
            }
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
    //KLOG_DEBUG() << "msgReply " << msgReply;
    QString errorMsg;

    if (msgReply.type() == QDBusMessage::ReplyMessage)
    {
        QList<QVariant> args = msgReply.arguments();
        if (args.size() >= 1)
        {
            QVariant firstArg = args.takeFirst();
            QJsonParseError jsonerror;
            QJsonDocument doc = QJsonDocument::fromJson(firstArg.toString().toLatin1(), &jsonerror);
            if (doc.isNull() || jsonerror.error != QJsonParseError::NoError || !doc.isObject())
            {
                KLOG_INFO() << "parse json err" << jsonerror.error;
                return false;
            }

            QJsonObject jsonObj = doc.object();
            for (auto key : jsonObj.keys())
            {
                if ("activation_status" == key)
                {
                    int activation_status = jsonObj[key].toDouble();
                    return activation_status != LicenseActivationStatus::LAS_ACTIVATED ? false : true;
                }
            }

            return false;
        }
    }

    return false;
}

void Monitor::clearSessionInfos()
{
    for (auto it = m_sessionInfos.begin(); it != m_sessionInfos.end();)
    {
        auto &process = it.value().process;
        m_sessionInfos.erase(it++);
        clearProcess(process);
    }

    KLOG_INFO() << "remove all end";
}

void Monitor::clearProcess(QProcess *process)
{
    if (process)
    {
        KLOG_INFO() << "call exit signal, remove "<< process->processId();
        MonitorDisk::instance().sendSwitchControl(process->processId(), "exit");
        if (process->waitForFinished())
        {
            process->close();
            delete process;
            process = nullptr;
        }
        KLOG_INFO() << "remove success";
    }
}
