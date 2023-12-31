#include "kiran-log/qt5-log-i.h"
#include "monitor.h"
#include "monitor-disk.h"
#include "common-definition.h"
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

Monitor::Monitor(QObject *parent)
{
    creatLicenseObjectName();
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

    clearFrontRecordInfos();
    for (auto it = m_frontHomeInfo.begin(); it != m_frontHomeInfo.end(); ++it)
    {
        MonitorDisk::instance().checkFrontRecordFreeSpace(it.key(), it.value());
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
                if (OPERATE_DISK_NOTIFY == message)
                {
                    it.value().bNotify = true;
                    KLOG_INFO() << "audit receive disk space notify";
                    return;
                }
                if (VAUDIT_ACTIVE == message)
                {
                    KLOG_INFO() << "audit send is_active";
                    it.value().bActive = true;
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
    if (authFile.isEmpty())
    {
        authFile = frontGetXAuth(userName, displayName);
    }

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
    // 规避前台录屏进程起多个，关掉旧的进程
    auto iter = m_frontRecordInfo.find(from_pid);
    if (iter != m_frontRecordInfo.end())
    {
        auto &pp = iter.value();
        KLOG_INFO() << "clear the vaudit process corresponding to process: " << iter.key();
        clearProcess(pp);
    }
    // 记录前后台进程信息
    m_frontRecordInfo.insert(from_pid, process);

    // 记录前台录屏目录信息：将同一存储目录的进程记录到一起
    QVector<int> vec;
    auto it = m_frontHomeInfo.find(homeDir);
    if (it != m_frontHomeInfo.end())
    {
        vec.append(it.value());
    }

    vec.append(from_pid);
    m_frontHomeInfo.insert(homeDir, vec);
}

// 获取本地会话的tty和用户名
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

    process.close();
    return map;
}

// 获取远程会话的进程ID，vnc和rdp
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

    process.close();
    return strlist;
}

// 获取本地会话信息，并根据tty获取对应会话的用户名
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

            struct sessionInfo info = {it.value(), VAUDIT_LOCAL_IP, arr[index1+1], arr[index2 + 1], nullptr, false, false, time(nullptr), true, false};
            vecInfo.push_back(info);
        }
    }

    process.close();
    return vecInfo;
}

// 获取远程会话对应IP，rdp第一次取到的IP为127.0.0.1，需要继续取
QString Monitor::getRemotIP(const QString &pid)
{
    QProcess process;
    process.setProgram("sh");
    QString arg = "IPADDR=$(netstat -antp | grep "+pid+" | grep ESTABLISHED | awk '{print $5}' | awk -F: '{print $1}' | tr -d '\n');"
                  "if [[ $IPADDR != '127.0.0.1' ]]; then"
                  "    echo -n $IPADDR;"
                  "    exit 0;"
                  "fi;"
                  "PORTNUM=$(netstat -antp | grep "+pid+" | grep ESTABLISHED | awk '{print $5}' | awk -F: '{print $2}' | tr -d '\n');"
                  "PIDNUM=$(netstat -antp | grep ${PORTNUM} | grep xrdp | awk '{print $7}' | cut -d/ -f1 | tr -d '\n');"
                  "IPADDR=$(netstat -antp | grep ${PIDNUM} | grep ESTABLISHED | grep -v '127.0.0.1' | awk '{print $5}' | cut -d: -f1 | tr -d '\n');"
                  "echo -n $IPADDR;";
    process.setArguments(QStringList() << "-c" << arg);
    process.start();
    if (process.waitForFinished())
    {
        QString resOut = process.readAllStandardOutput().toStdString().data();
        if (!resOut.isEmpty())
        {
            process.close();
            return resOut;
        } else {
            QString resErr = process.readAllStandardError().toStdString().data();
//        KLOG_INFO() << "Get remote ip ERROR: [" << resErr << "]";
        }
    }
    process.close();
    return "";
}

// 获取远程会话信息
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

            // vnc: -auth, rdp: -rfbauth
            int index = strlist.indexOf(QRegularExpression(".*Xvnc$"));
            int index1 = strlist.indexOf(QRegularExpression("^-auth"));
            int index2 = index1 == -1 ? strlist.indexOf(QRegularExpression("^-rfbauth")) : index1;
            if (index == -1 || index2 == -1)
                continue;

            struct sessionInfo info = {user, ip, strlist[index+1], strlist[index2+1], nullptr, false, false, time(nullptr), false, false};
            vecInfo.push_back(info);
        }
        process.close();
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

    // 62419
    if (info.userName != "root") {
        QString authpath = "/home/" + info.userName + "/.Xauthority";
        QFile authfile(authpath);
        if (authfile.exists()) {
            info.authFile = "/home/" + info.userName + "/.Xauthority";
        }
    }
 
    env.insert("XAUTHORITY", info.authFile);
    process->setProcessEnvironment(env);
    QStringList arg;
    arg << (QString("--audit ") + info.userName + QString(" ") + info.ip + QString(" ") + info.displayName);

    // 进程异常退出，重新拉起
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
                KLOG_INFO() << "not find, killed process start time" << value.stTime << "find time" << tmp.stTime;
                return;
            }

            KLOG_INFO() << "restart bin, find" << it.key() << value.ip << value.displayName << value.authFile << value.stTime;
            auto &pp = value.process;
            if (pp)
            {
                pp->close();
                delete pp;
                pp = nullptr;
            }

        #if 1
            QString displayName = getCurrentSessionDisplay();
            // 本地未激活用户不启进程，没有获取到未激活用户当正常情况处理
            if (info.bLocal && !displayName.isEmpty() && value.displayName != displayName)
            {
                KLOG_INFO() << value.userName << value.displayName << "is not active user" << "cur display:" << displayName;
                m_sessionInfos.erase(it);
                return;
            }
        #endif

            value.stTime = time(nullptr);
            value.bStart = false;
            value.bActive = false;
            value.bNotify = false;
            value.process = startRecordWithDisplay(value);
            KLOG_INFO() << "deal CrashExit end" << value.stTime;
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
    QString loginDisplay;
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

        #if 1
            // 获取激活的会话，对于本对只会有一个会话正在使用，一次循环内仅需获取一次
            if (loginDisplay.isEmpty())
            {
                loginDisplay = getCurrentSessionDisplay();
            }
            // 本地未激活用户不启进程，没有获取到未激活用户当正常情况处理
            if (info.bLocal && !loginDisplay.isEmpty() && info.displayName != loginDisplay)
                continue;
        #endif

            mapCnt.insert(info.userName, ++mapCnt[info.userName]);
            info.process = startRecordWithDisplay(info);
            m_sessionInfos.insert(info.userName, info);
        }
    }

    for (auto it = m_sessionInfos.begin(); it != m_sessionInfos.end();)
    {
        auto &val = it.value();
        // 磁盘空间正常
        if (isDiskOk)
        {
            if (!val.bActive)
            {
                if (time(nullptr) - val.stTime > 10)
                {
                    // 约10s没收到子进程的已激活信息，认为子进程卡住，杀掉子进程并清除信息
                    auto &process = it.value().process;
                    KLOG_INFO() << "about 10s to start time:" << val.stTime << ", kill process" << process->pid() << ", displayName:" << val.displayName;
                    m_sessionInfos.erase(it++);
                    process->kill();
                    process->waitForFinished();
                    process->deleteLater();
                    KLOG_INFO() << "remove success";
                    continue;
                }
            }

            // 进程存在，但是没有录屏(以收到录屏文件为准)，发送开始录屏信息
            if (!val.bStart)
            {
                KLOG_INFO() << "record not start and start, pid:" << val.process->pid() << "display:" << val.displayName << "ip:" << val.ip;

                // 清除不存在进程信息
                if (0 == val.process->pid())
                {
                    auto process = it.value().process;
                    m_sessionInfos.erase(it++);
                    clearProcess(process);
                    continue;
                }

                MonitorDisk::instance().sendSwitchControl(val.process->pid(), OPERATE_RECORD_START);
            }

            // 关闭提示磁盘空间不足
            if (val.bNotify)
            {
                KLOG_INFO() << "disk space returned to normal, stop notify, pid:" << val.process->pid();
                MonitorDisk::instance().sendSwitchControl(val.process->pid(), OPERATE_DISK_NOTIFY_STOP);
                val.bNotify = false;
            }
        }
        else
        {
            //磁盘空间不足处理，如果停止过录像并正在提示不处理
            if (!val.bNotify)
            {
                KLOG_INFO() << "insufficient disk space, stop record and notify pid:" << val.process->processId();
                MonitorDisk::instance().sendSwitchControl(val.process->processId(), OPERATE_RECORD_STOP);
                MonitorDisk::instance().sendSwitchControl(val.process->processId(), OPERATE_DISK_NOTIFY);
                val.bStart = false;
            }
        }

        ++it;
    }
}

bool Monitor::isLicenseActive()
{
    QDBusMessage msgMethodCall = QDBusMessage::createMethodCall(DBUS_LICENSE_DESTIONATION,
                                                                DBUS_LICENSE_VAUDIT_PATH,
                                                                DBUS_LICENSE_INTERFACE,
                                                                DBUS_LICENSE_METHOD_GETLICENSE);
    QDBusMessage msgReply = QDBusConnection::systemBus().call(msgMethodCall, QDBus::Block, DBUS_TIMEOUT_MS);
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
            auto iter = jsonObj.find(DBUS_LICENSE_ACTIVATION_STATUS);
            if (iter != jsonObj.end())
            {
                return iter.value().toDouble() == 1 ? true : false;
            }

            return false;
        }
    }

    return false;
}

// 清除后台所有会话信息
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

// 退出后台进程
void Monitor::clearProcess(QProcess *process)
{
    if (process)
    {
        KLOG_INFO() << "call exit signal, remove "<< process->processId();
        MonitorDisk::instance().sendSwitchControl(process->processId(), OPERATE_RECORD_EXIT);
        if (process->waitForFinished())
        {
            process->close();
            delete process;
            process = nullptr;
        }
        KLOG_INFO() << "remove success";
    }
}

void Monitor::clearFrontRecordInfos()
{
    // 查询前台进程是否还存在，不存在关闭后台录屏进程，清除信息
    for (auto it = m_frontRecordInfo.begin(); it != m_frontRecordInfo.end();)
    {
        bool bExist = processExist(it.key());
        if (!bExist)
        {
            KLOG_INFO() << "front pid:" << it.key() << "is not exist";
            // 清除文件存储路径信息, pid是唯一的
            for (auto iter = m_frontHomeInfo.begin(); iter != m_frontHomeInfo.end(); ++iter)
            {
                QVector<int> &vec = iter.value();
                int index = vec.indexOf(it.key());
                // 查找到，清除信息，没找到继续
                if (index != -1)
                {
                    vec.remove(index);
                    // 没有存放到对应home的进程
                    if (vec.size() == 0)
                    {
                        m_frontHomeInfo.erase(iter++);
                    }
                    break;
                }
            }

            auto &pp = it.value();
            m_frontRecordInfo.erase(it++);
            clearProcess(pp);
            continue;
        }
        ++it;
    }
}

// 获取前台会话XAUTHORITY
QString Monitor::frontGetXAuth(QString userName, QString display)
{
    int displayIndex = display.indexOf(".") ;
    QString displayName = displayIndex == -1 ? display.mid(0, display.size()) : display.mid(0, displayIndex);
    KLOG_INFO() << "displayName" << displayName;

    // 先在后台的数据里查
    QList<sessionInfo> values = m_sessionInfos.values(userName);
    for (auto value : values)
    {
        if (value.displayName == displayName)
        {
            KLOG_INFO() << "display" << displayName << "find m_sessionInfos get XAUTHORITY:" << value.authFile;
            return value.authFile;
        }
    }

    // 通过命令查找
    QProcess process;
    process.setProgram("sh");
    QString arg = "pid=`ps aux | grep " + userName + " | grep -E 'Xorg " + displayName + "|Xvnc " + displayName + "' | grep -v grep | awk '{print $2}'` &&  cat /proc/$pid/cmdline";
    KLOG_INFO() << "arg:" << arg;
    process.setArguments(QStringList() << "-c" << arg);
    process.start();
    QString resOut;
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
        int index1 = strlist.indexOf(QRegularExpression("^-auth"));
        int index = index1 == -1 ? strlist.indexOf(QRegularExpression("^-rfbauth")) : index1;
        resOut = index == -1 ? resOut :  strlist[index+1];
    }

    KLOG_INFO() << "display" << displayName << "get XAUTHORITY:" << resOut;
    process.close();
    return resOut;
}

// 判断进程是否存在
bool Monitor::processExist(int pid)
{
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.setProgram("sh");
    QString arg = "ps -p " + QString::number(pid) + " >/dev/null 2>&1; echo $?";
    process.setArguments(QStringList() << "-c" << arg);
    process.start();
    bool bExist{};
    if (process.waitForFinished())
    {
        QByteArray data = process.readAll();
        QString str(data.toStdString().data());
        QStringList strlist = str.split("\n");
        strlist.removeAll("");
        if (strlist.size() == 1 &&  strlist[0].toInt() == 0)
        {
            bExist = true;
        }
    }
    process.close();
    return bExist;
}

// dbus-send --system --print-reply --type=method_call --dest=org.freedesktop.ConsoleKit /org/freedesktop/ConsoleKit/Seat1 org.freedesktop.ConsoleKit.Seat.GetActiveSession
// dbus-send --system --print-reply --type=method_call --dest=org.freedesktop.ConsoleKit /org/freedesktop/ConsoleKit/Session2 org.freedesktop.ConsoleKit.Session.GetX11Display
QString Monitor::getCurrentSessionDisplay()
{
    QString displayName;
    QDBusMessage msgMethodCall = QDBusMessage::createMethodCall("org.freedesktop.ConsoleKit",
                                                                "/org/freedesktop/ConsoleKit/Seat1",
                                                                "org.freedesktop.ConsoleKit.Seat",
                                                                "GetActiveSession");
    QDBusMessage msgReply = QDBusConnection::systemBus().call(msgMethodCall, QDBus::Block, DBUS_TIMEOUT_MS);
    KLOG_DEBUG() << "msgReply " << msgReply;
    QString session;
    if (msgReply.type() != QDBusMessage::ReplyMessage)
        return displayName;

    QList<QVariant> args = msgReply.arguments();
    if (args.size() < 1)
        return displayName;

    QVariant firstArg = args.takeFirst();
    if (firstArg.canConvert<QDBusObjectPath>())
    {
        QDBusObjectPath arg = firstArg.value<QDBusObjectPath>();
        session = arg.path();
    }

    if (session.isEmpty())
        return displayName;


    QDBusMessage msgMethodCall1 = QDBusMessage::createMethodCall("org.freedesktop.ConsoleKit",
                                                                session,
                                                                "org.freedesktop.ConsoleKit.Session",
                                                                "GetX11Display");
    QDBusMessage msgReply1 = QDBusConnection::systemBus().call(msgMethodCall1, QDBus::Block, DBUS_TIMEOUT_MS);
    KLOG_DEBUG() << session << "GetX11AuthorityFile msgReply" << msgReply1;

    if (msgReply1.type() != QDBusMessage::ReplyMessage)
        return displayName;

    QList<QVariant> args1 = msgReply1.arguments();
    if (args1.size() < 1)
        return displayName;

    displayName = args1.takeFirst().toString();
    KLOG_DEBUG() << "current session " << session << "displayName:" << displayName;
    return displayName;
}

bool Monitor::creatLicenseObjectName()
{
    QDBusMessage msgMethodCall = QDBusMessage::createMethodCall(DBUS_LICENSE_DESTIONATION,
                                                                DBUS_LICENSE_OBJECT_PATH,
                                                                DBUS_LICENSE_DESTIONATION,
                                                                DBUS_LICENSE_METHOD_OBJECK);
    msgMethodCall << DBUS_LICENSE_OBJECT_VAUDIT_NAME;

    QDBusMessage msgReply = QDBusConnection::systemBus().call(msgMethodCall, QDBus::Block, DBUS_TIMEOUT_MS);
    KLOG_INFO() << "msgReply:" << msgReply;
    if (msgReply.type() != QDBusMessage::ReplyMessage)
        return false;

    QList<QVariant> args = msgReply.arguments();
    if (args.size() < 1)
        return false;

    return true;
}
