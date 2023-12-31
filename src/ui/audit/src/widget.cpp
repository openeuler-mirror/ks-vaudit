#include "widget.h"
#include "ui_widget.h"
#include <QtDebug>
#include <QList>
#include <QFont>
#include <QString>
#include <QFileDialog>
#include <QMenu>
#include <QListView>
#include <QDateTime>
#include <QDesktopServices>
#include <QUrl>
#include <QHBoxLayout>
#include <QDBusMessage>
#include <QDBusConnection>
#include "kiran-log/qt5-log-i.h"
#include "ksvaudit-configure_global.h"
#include "common-definition.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/pixdesc.h"
#include "libavutil/pixfmt.h"
#include "libavutil/dict.h"
}

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
    m_dbusInterface = new ConfigureInterface(KSVAUDIT_CONFIGURE_SERVICE_NAME, KSVAUDIT_CONFIGURE_PATH_NAME, QDBusConnection::systemBus(), this);
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, true);
    m_activatedPage = new ActivatePage(this);
    getActivationInfo();
//    this->init_ui();
    if (m_fpsList.isEmpty()){
        m_fpsList << "5" << "10" << "20" << "25" << "30" << "45" << "60";
    }
    connect(ui->acceptBtn,SIGNAL(clicked()),this,SLOT(setConfig()));
    connect(ui->cancelBtn,SIGNAL(clicked()),this,SLOT(resetConfig()));

}

Widget::~Widget()
{
    delete ui;
    if (m_dbusInterface){
        delete m_dbusInterface;
        m_dbusInterface = nullptr;
    }
    if (m_model){
        m_model->clear();
        delete m_model;
        m_model = nullptr;
    }
}


void Widget::init_ui()
{
    ui->ListBody->hide();
    ui->UserBody->hide();
    ui->ConfigBody->hide();
    ui->AboutBody->hide();
    ui->EditPassWidget->hide();

    ui->listArrow->hide();
    ui->userArrow ->hide();
    ui->configArrow->hide();
    ui->aboutArrow->hide();


    ui->ListBtn->setStyleSheet("QPushButton#ListBtn{"
                               "border: 0px;"
                               "background-color:rgb(57,57,57);"
                               "color: rgb(255, 255, 255);"
                               "border-radius: 6px;"
                               "text-align:left center;"
                               "padding-left:10px;"
                               "};");
    ui->ConfigBtn->setStyleSheet("QPushButton#ConfigBtn{"
                                 "border: 0px;"
                                 "background-color:rgb(57,57,57);"
                                 "color: rgb(255, 255, 255);"
                                 "border-radius: 4px;"
                                 "text-align:left center;"
                                 "padding-left:10px;"
                                 "};");
    ui->UserBtn->setStyleSheet("QPushButton#UserBtn{"
                        "border: 0px;"
                        "background-color:rgb(57,57,57);"
                        "color: rgb(255, 255, 255);"
                        "border-radius: 4px;"
                        "text-align:left center;"
                        "padding-left:10px;"
                        "};");
    ui->AboutBtn->setStyleSheet("QPushButton#AboutBtn{"
                        "border: 0px;"
                        "background-color:rgb(57,57,57);"
                        "color: rgb(255, 255, 255);"
                        "border-radius: 4px;"
                        "text-align:left center;"
                        "padding-left:10px;"
                        "};");

    ui->oldPasswordEdit->setProperty("isError", false);
    ui->oldPasswordEdit->clear();
    ui->newPasswordEdit->setProperty("isError", false);
    ui->newPasswordEdit->clear();
    ui->confirmPasswordEdit->setProperty("isError", false);
    ui->confirmPasswordEdit->clear();
    ui->label_24->hide();
    ui->label_26->hide();
    ui->label_25->clear();

    readConfig();
    setConfigtoUi();
    ui->acceptBtn->hide();
    ui->cancelBtn->hide();
}

void Widget::initAudadmUi()
{
    // 只能访问列表、用户、关于
    init_ui();
    ui->ListBody->show();
    ui->listArrow->show();
    ui->ListBtn->setStyleSheet("QPushButton#ListBtn{"
                               "border: 0px;"
                               "background-color:#2eb3ff;"
                               "color: rgb(255, 255, 255);"
                               "border-radius: 6px;"
                               "text-align:left center;"
                               "padding-left:10px;"
                               "};");

    // 列表右键弹出菜单
    m_rightMenu = new QMenu(this);
    m_rightMenu->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
    m_rightMenu->setAttribute(Qt::WA_TranslucentBackground);
    m_rightMenu->setObjectName("listMenu");
    m_playAction = m_rightMenu->addAction(tr("Play"));
    m_folderAction = m_rightMenu->addAction(tr("File Location"));

    connect(m_playAction, SIGNAL(triggered()), this, SLOT(playVideo()));
    connect(m_folderAction, SIGNAL(triggered()), this, SLOT(openDir()));

    // 构建模型表头
    m_model = new QStandardItemModel();
    createList();
    refreshList(m_regName);
}

void Widget::initSysadmUi()
{
    init_ui();
    ui->ConfigBody->show();
    ui->configArrow->show();
    ui->ConfigBtn->setStyleSheet("QPushButton#ConfigBtn{"
                               "border: 0px;"
                               "background-color:#2eb3ff;"
                               "color: rgb(255, 255, 255);"
                               "border-radius: 6px;"
                               "text-align:left center;"
                               "padding-left:10px;"
                               "};");

    this->comboboxStyle();
}

void Widget::initSecadmUi()
{
    init_ui();
    ui->UserBody->show();
    ui->userArrow->show();
    ui->UserBtn->setStyleSheet("QPushButton#UserBtn{"
                               "border: 0px;"
                               "background-color:#2eb3ff;"
                               "color: rgb(255, 255, 255);"
                               "border-radius: 6px;"
                               "text-align:left center;"
                               "padding-left:10px;"
                               "};");
}

void Widget::comboboxStyle(){
    ui->clarityBox->setView(new QListView());
    ui->fpsBox->setView(new QListView());
    ui->typeBox->setView(new QListView());
    ui->pauseBox->setView(new QListView());
    ui->freeSpaceBox->setView(new QListView());
    ui->maxRecordBox->setView(new QListView());
    ui->keepTimeBox->setView(new QListView());
}


void Widget::mousePressEvent(QMouseEvent *event)
{
    if (event->pos().y() < 40 && event->button() == Qt::LeftButton){
        m_bDrag = true;
        mouseStartPoint = event->globalPos();
        windowTopLeftPoint = this->frameGeometry().topLeft();
    }
}

void Widget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_bDrag){
        QPoint  distance = event->globalPos() - mouseStartPoint;
        this->move(windowTopLeftPoint + distance);
    }
}

void Widget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton){
        m_bDrag = false;
    }
}

void Widget::on_exit_clicked()
{
    // 审计不需要确认，直接退出
//    Dialog *confirm = new Dialog(this, QString("exit"));
//    connect(confirm, SIGNAL(close_window()), this, SLOT(realClose()));
//    confirm->exec();
    this->close();
}


void Widget::on_ListBtn_clicked()
{
    if (m_currentUserInfo["user"].toString() != UI_AUDADM_USERNAME){
        Dialog *d = new Dialog(this,"noprivilege");
        d->exec();
        return;
    }
    ui->ConfigBody->hide();
    ui->configArrow->hide();
    ui->UserBody->hide();
    ui->userArrow->hide();
    ui->AboutBody->hide();
    ui->aboutArrow->hide();
    ui->EditPassWidget->hide();

    ui->ListBody->show();
    ui->listArrow->show();

    ui->ListBtn->setStyleSheet("QPushButton#ListBtn{"
                               "border: 0px;"
                               "background-color:#2eb3ff;"
                               "color: rgb(255, 255, 255);"
                               "border-radius: 6px;"
                               "text-align:left center;"
                               "padding-left:10px;"
                               "};");
    ui->ConfigBtn->setStyleSheet("QPushButton#ConfigBtn{"
                                 "border: 0px;"
                                 "background-color:rgb(57,57,57);"
                                 "color: rgb(255, 255, 255);"
                                 "border-radius: 4px;"
                                 "text-align:left center;"
                                 "padding-left:10px;"
                                 "};");
    ui->UserBtn->setStyleSheet("QPushButton#UserBtn{"
                        "border: 0px;"
                        "background-color:rgb(57,57,57);"
                        "color: rgb(255, 255, 255);"
                        "border-radius: 4px;"
                        "text-align:left center;"
                        "padding-left:10px;"
                        "};");
    ui->AboutBtn->setStyleSheet("QPushButton#AboutBtn{"
                        "border: 0px;"
                        "background-color:rgb(57,57,57);"
                        "color: rgb(255, 255, 255);"
                        "border-radius: 4px;"
                        "text-align:left center;"
                        "padding-left:10px;"
                        "};");
    refreshList(m_regName);
}

void Widget::on_ConfigBtn_clicked()
{
    if (m_currentUserInfo["user"].toString() != UI_SYSADM_USERNAME){
        Dialog *d = new Dialog(this,"noprivilege");
        d->exec();
        return;
    }
    ui->configArrow->show();
    ui->ConfigBody->show();

    ui->ListBody->hide();
    ui->listArrow->hide();
    ui->userArrow->hide();
    ui->UserBody->hide();
    ui->AboutBody->hide();
    ui->aboutArrow->hide();
    ui->EditPassWidget->hide();

    ui->ListBtn->setStyleSheet("QPushButton#ListBtn{"
                                 "border: 0px;"
                                 "background-color:rgb(57,57,57);"
                                 "color: rgb(255, 255, 255);"
                                 "border-radius: 4px;"
                                 "text-align:left center;"
                                 "padding-left:10px;"
                                 "};");
    ui->ConfigBtn->setStyleSheet("QPushButton#ConfigBtn{"
                               "border: 0px;"
                               "background-color:#2eb3ff;"
                               "color: rgb(255, 255, 255);"
                               "border-radius: 6px;"
                               "text-align:left center;"
                               "padding-left:10px;"
                               "};");
    ui->UserBtn->setStyleSheet("QPushButton#UserBtn{"
                        "border: 0px;"
                        "background-color:rgb(57,57,57);"
                        "color: rgb(255, 255, 255);"
                        "border-radius: 4px;"
                        "text-align:left center;"
                        "padding-left:10px;"
                        "};");
    ui->AboutBtn->setStyleSheet("QPushButton#AboutBtn{"
                        "border: 0px;"
                        "background-color:rgb(57,57,57);"
                        "color: rgb(255, 255, 255);"
                        "border-radius: 4px;"
                        "text-align:left center;"
                        "padding-left:10px;"
                        "};");
}

void Widget::on_UserBtn_clicked()
{
    ui->userArrow->show();
    ui->UserBody->show();

    ui->ListBody->hide();
    ui->listArrow->hide();
    ui->configArrow->hide();
    ui->ConfigBody->hide();
    ui->AboutBody->hide();
    ui->aboutArrow->hide();


    ui->ListBtn->setStyleSheet("QPushButton#ListBtn{"
                        "border: 0px;"
                        "background-color:rgb(57,57,57);"
                        "color: rgb(255, 255, 255);"
                        "border-radius: 4px;"
                        "text-align:left center;"
                        "padding-left:10px;"
                        "};");
    ui->ConfigBtn->setStyleSheet("QPushButton#ConfigBtn{"
                                 "border: 0px;"
                                 "background-color:rgb(57,57,57);"
                                 "color: rgb(255, 255, 255);"
                                 "border-radius: 4px;"
                                 "text-align:left center;"
                                 "padding-left:10px;"
                                 "};");
    ui->UserBtn->setStyleSheet("QPushButton#UserBtn{"
                        "border: 0px;"
                        "background-color:#2eb3ff;"
                        "color: rgb(255, 255, 255);"
                        "border-radius: 4px;"
                        "text-align:left center;"
                        "padding-left:10px;"
                        "};");
    ui->AboutBtn->setStyleSheet("QPushButton#AboutBtn{"
                        "border: 0px;"
                        "background-color:rgb(57,57,57);"
                        "color: rgb(255, 255, 255);"
                        "border-radius: 4px;"
                        "text-align:left center;"
                        "padding-left:10px;"
                        "};");
}

void Widget::on_AboutBtn_clicked()
{
    ui->AboutBody->show();
    ui->aboutArrow->show();

    ui->ListBody->hide();
    ui->listArrow->hide();
    ui->ConfigBody->hide();
    ui->configArrow->hide();
    ui->UserBody->hide();
    ui->userArrow->hide();
    ui->EditPassWidget->hide();

    ui->ListBtn->setStyleSheet("QPushButton#ListBtn{"
                        "border: 0px;"
                        "background-color:rgb(57,57,57);"
                        "color: rgb(255, 255, 255);"
                        "border-radius: 4px;"
                        "text-align:left center;"
                        "padding-left:10px;"
                        "};");


    ui->ConfigBtn->setStyleSheet("QPushButton#ConfigBtn{"
                                 "border: 0px;"
                                 "background-color:rgb(57,57,57);"
                                 "color: rgb(255, 255, 255);"
                                 "border-radius: 4px;"
                                 "text-align:left center;"
                                 "padding-left:10px;"
                                 "};");

    ui->UserBtn->setStyleSheet("QPushButton#UserBtn{"
                        "border: 0px;"
                        "background-color:rgb(57,57,57);"
                        "color: rgb(255, 255, 255);"
                        "border-radius: 4px;"
                        "text-align:left center;"
                        "padding-left:10px;"
                        "};");

    ui->AboutBtn->setStyleSheet("QPushButton#AboutBtn{"
                        "border: 0px;"
                        "background-color:#2eb3ff;"
                        "color: rgb(255, 255, 255);"
                        "border-radius: 4px;"
                        "text-align:left center;"
                        "padding-left:10px;"
                        "};");
}


void Widget::on_pushButton_clicked()
{
    QString dirPath;
    QString homePath = QDir::homePath();
    QString openDir = ui->pathLabel->text();
    if (openDir.startsWith("~")){
        openDir.replace(0,1,homePath);
    }
    dirPath = QFileDialog::getExistingDirectory(this, tr("Please select file path ..."), openDir, QFileDialog::ShowDirsOnly);
    if (!dirPath.isEmpty()){
        ui->pathLabel->setText(dirPath);
        ui->pathLabel->setToolTip(dirPath);
        QJsonObject obj;
        obj[GENEARL_CONFIG_FILEPATH] = dirPath;
        QJsonDocument doc(obj);
        QString a = QString::fromUtf8(doc.toJson(QJsonDocument::Compact).constData());
        m_dbusInterface->SetAuditItemValue(a);
        refreshList(m_regName);
    }else{
        KLOG_DEBUG("Invalid path, abort!");
    }

}

void Widget::onTableBtnClicked()
{
    // 从属性里获取index
    int m = sender()->property("M_INDEX").toInt();
    int l = sender()->property("L_INDEX").toInt();

    // 选择行
    ui->videoList->selectRow(l);

    // 获取视频
//    QList<QFileInfo>* testList = getVideos(ui->pathLabel->text(),m_regName);
    m_folderAction->setProperty("S_DIR", m_fileList.at(m).absolutePath());
    m_playAction->setProperty("S_FILEPATH", m_fileList.at(m).filePath());

    m_rightMenu->popup(QCursor::pos());
}


void Widget::on_minimize_clicked()
{
    Widget::showMinimized();
}

void Widget::on_fpsBox_currentIndexChanged(int index)
{
    testConfig(GENEARL_CONFIG_FPS, m_fpsList.at(index));
}

void Widget::on_searchBar_returnPressed()
{
    m_regName = ui->searchBar->text();
    refreshList(m_regName);
    if (ui->searchBar->hasFocus()){
        ui->searchBar->clearFocus();
    }
}

void Widget::on_clarityBox_currentIndexChanged(int index)
{
    testConfig(GENEARL_CONFIG_QUALITY, QString("%1").arg(index));
}

void Widget::on_pauseBox_currentIndexChanged(int index)
{
    int setValue = TIMING_PAUSE_LEVEL_0;
    if (UI_INDEX_LEVEL_1 == index){
        setValue = TIMING_PAUSE_LEVEL_1;
    }else if (UI_INDEX_LEVEL_2 == index){
        setValue = TIMING_PAUSE_LEVEL_2;
    }else if (UI_INDEX_LEVEL_3 == index){
        setValue = TIMING_PAUSE_LEVEL_3;
    }else{
        setValue = TIMING_PAUSE_LEVEL_0;
    }
    testConfig(QString(GENEARL_CONFIG_TIMING_PAUSE), QString("%1").arg(setValue));
}

void Widget::on_typeBox_currentIndexChanged(int index)
{
    QString setValue = QString(UI_FILETYPE_MKV);
    if (UI_INDEX_LEVEL_1 == index) {
        setValue = QString(UI_FILETYPE_MP4);
    }

    testConfig(QString(GENEARL_CONFIG_FILETYPE), setValue);
}

void Widget::on_freeSpaceBox_currentIndexChanged(int index)
{
    unsigned long long setValue = AUDIT_DISK_LEVEL_1;
    if (UI_INDEX_LEVEL_0 == index){
        setValue = AUDIT_DISK_LEVEL_0;
    }else if (UI_INDEX_LEVEL_2 == index){
        setValue = AUDIT_DISK_LEVEL_2;
    }else if (UI_INDEX_LEVEL_3 == index){
        setValue = AUDIT_DISK_LEVEL_3;
    }else{
        setValue = AUDIT_DISK_LEVEL_1;
    }
    testConfig(QString(GENEARL_CONFIG_MIN_FREE_SPACE), QString("%1").arg(setValue));
}

void Widget::on_keepTimeBox_currentIndexChanged(int index)
{
    int setValue = SAVE_MAX_DAYS_LEVEL_0;
    if (UI_INDEX_LEVEL_1 == index){
        setValue = SAVE_MAX_DAYS_LEVEL_1;
    }else if (UI_INDEX_LEVEL_2 == index){
        setValue = SAVE_MAX_DAYS_LEVEL_2;
    }else if (UI_INDEX_LEVEL_3 == index){
        setValue = SAVE_MAX_DAYS_LEVEL_3;
    }else{
        setValue = SAVE_MAX_DAYS_LEVEL_0;
    }
    testConfig(QString(GENEARL_CONFIG_MAX_SAVE_DAYS), QString("%1").arg(setValue));
}

void Widget::on_maxRecordBox_currentIndexChanged(int index)
{
    int setValue = MAX_RECORD_PER_USER_LEVEL_0;
    if (UI_INDEX_LEVEL_1 == index){
        setValue = MAX_RECORD_PER_USER_LEVEL_1;
    }else if (UI_INDEX_LEVEL_2 == index){
        setValue = MAX_RECORD_PER_USER_LEVEL_2;
    }else if (UI_INDEX_LEVEL_3 == index){
        setValue = MAX_RECORD_PER_USER_LEVEL_3;
    }else{
        setValue = MAX_RECORD_PER_USER_LEVEL_0;
    }
    testConfig(QString(GENEARL_CONFIG_MAX_RECORD_PER_USER), QString("%1").arg(setValue));
}

void Widget::on_editPasswordBtn_clicked()
{
    ui->EditPassWidget->show();
    ui->UserBody->hide();
}

void Widget::on_cancelPasswordBtn_clicked()
{
    ui->EditPassWidget->hide();
    ui->UserBody->show();
}

void Widget::on_savePasswordBtn_clicked()
{
    bool hasError = false;
    QString old_pw = ui->oldPasswordEdit->text();
    QString new_pw = ui->newPasswordEdit->text();
    QString cf_pw = ui->confirmPasswordEdit->text();
    QString chk_pw = checkNewPassword(new_pw);
    if (old_pw != base64ToStr(m_currentUserInfo["passwd"].toString())){
        hasError = true;
        ui->oldPasswordEdit->setProperty("isError", true);
        ui->oldPasswordEdit->style()->polish(ui->oldPasswordEdit);
        ui->label_24->show();
    }
    if (chk_pw.length() != 0){
        hasError = true;
        ui->newPasswordEdit->setProperty("isError", true);
        ui->newPasswordEdit->style()->polish(ui->newPasswordEdit);
        ui->label_25->setText(chk_pw);
    }
    if (QString::compare(new_pw,cf_pw,Qt::CaseSensitive) != 0){
        hasError = true;
        ui->confirmPasswordEdit->setProperty("isError", true);
        ui->confirmPasswordEdit->style()->polish(ui->confirmPasswordEdit);
        ui->label_26->show();
    }
    if (!hasError){
        // {"user":"test", "old_passwd"="123", role="sysadm", "passwd"="1234"}
        QString toSetInfo = QString("{\"user\":\"%1\", \"old_passwd\":\"%2\", \"role\":\"%3\", \"passwd\":\"%4\"}")
                .arg(m_currentUserInfo["user"].toString())
                .arg(strTobase64(old_pw))
                .arg(m_currentUserInfo["role"].toString())
                .arg(strTobase64(new_pw));
        m_dbusInterface->ModifyUserInfo(toSetInfo);
        emit log_out();
        this->hide();
    }
}

void Widget::on_oldPasswordEdit_textChanged(const QString &arg1)
{
    ui->oldPasswordEdit->setProperty("isError", false);
    ui->oldPasswordEdit->style()->polish(ui->oldPasswordEdit);
    ui->label_24->hide();
}

void Widget::on_newPasswordEdit_textChanged(const QString &arg1)
{
    ui->newPasswordEdit->setProperty("isError", false);
    ui->newPasswordEdit->style()->polish(ui->newPasswordEdit);
    ui->label_25->clear();
}

void Widget::on_confirmPasswordEdit_textChanged(const QString &arg1)
{
    ui->confirmPasswordEdit->setProperty("isError", false);
    ui->confirmPasswordEdit->style()->polish(ui->confirmPasswordEdit);
    ui->label_26->hide();
}

void Widget::on_logoutBtn_clicked()
{
    emit log_out();
    this->hide();
}

QString Widget::checkNewPassword(QString inputpw)
{
    QString retString = "";
    QRegExp charReg = QRegExp("[\\x0020-\\x002f\\x003a-\\x0040\\x005b-\\x0060\\x007b-\\x007e]+");
    if (inputpw.length() > 30 || inputpw.length() < 8){
        retString = tr("Password length range should be 8-30");
    }else if(inputpw.contains(QRegExp("[\\x4e00-\\x9fa5]+"))){
        retString = tr("Password cannot contains any Chinese chars");
    }else if(!inputpw.contains(charReg)){
        retString = tr("Password must include a specical character");
    }else if(!inputpw.contains(QRegExp("[0-9]+"))){
        retString = tr("Password must include a digital number");
    }else if(!inputpw.contains(QRegExp("[a-zA-Z]+"))){
        retString = tr("Password must include a letter");
    }else{
        retString = "";
    }

    return retString;
}

void Widget::playVideo()
{
    QString filePath = sender()->property("S_FILEPATH").toString();
    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);
    QStringList commands;
    commands << filePath;
    // 采用分离式启动(startDetached)ffplay，因为和本录屏软件无关
    p.startDetached("ffplay", commands);
}

void Widget::openDir()
{
    QString videoPath = sender()->property("S_DIR").toString();
    QDesktopServices::openUrl(QUrl::fromLocalFile(videoPath));
}

QPushButton *Widget::createOperationBtn(int modelIndex, int listIndex)
{
    QPushButton *aBtn = new QPushButton();
    // 设置图标样式
    aBtn->setStyleSheet("QPushButton{"
                        "border:none;"
                        "background-color:transparent;"
                        "image:url(:/images/operationicon.svg);"
                        "padding-left:4px;"
                        "margin-top:7px;"
                        "}");
    aBtn->setMaximumSize(24,24);
    aBtn->setProperty("L_INDEX", listIndex);
    aBtn->setProperty("M_INDEX", modelIndex);

    connect(aBtn, SIGNAL(clicked(bool)), this, SLOT(onTableBtnClicked()));
    return aBtn;
}

QList<QFileInfo> Widget::getVideos(QString path, QString regName = QString(""))
{
    if (path.startsWith("~")){
        path = path.replace(0,1,QDir::homePath());
    }
    QDir dir(path);
    QStringList filters;
    filters << "*.mp4" << "*.mkv" << "*.ogv";
    dir.setNameFilters(filters);
    // 默认时间排序
    dir.setSorting(QDir::Time);
    dir.setFilter(QDir::Files | QDir::NoSymLinks);
    QList<QFileInfo> filesInfo(dir.entryInfoList());
    QList<QFileInfo> filesInfoRet;
    for (int i = 0; i < filesInfo.size(); ++i){
        if (regName.isEmpty()){
            break;
        }
        if (filesInfo.at(i).fileName().contains(regName)){
            filesInfoRet.append(filesInfo.at(i));
        }
    }
    return regName.isEmpty() ? filesInfo : filesInfoRet;
}

QString Widget::getVideoDuration(QString absPath)
{
    int secs = 0;
    int mins = 0;
    int hours = 0;
    AVFormatContext* pCtx = nullptr;
    int ret = avformat_open_input(&pCtx, absPath.toStdString().c_str(), nullptr, nullptr);
    if (ret < 0){
        KLOG_DEBUG() << "avformat_open_input() failed:" << ret;
        return QString(tr("File broken"));
    }
    int ret1 = avformat_find_stream_info(pCtx,nullptr);
    if(ret1 < 0){
        KLOG_DEBUG() << "avformat_find_stream_info() failed:" << ret1;
        if (pCtx){
            avformat_close_input(&pCtx);
            pCtx=nullptr;
        }
        return QString(tr("File broken"));
    }

    if (pCtx->duration != AV_NOPTS_VALUE){
        int64_t duration = pCtx->duration + (pCtx->duration <= INT64_MAX - 5000 ? 5000 : 0);
        secs = duration / AV_TIME_BASE;
        // #59083 过滤掉时长为0播放不了的视频
        if (secs == 0){
            if (pCtx){
                avformat_close_input(&pCtx);
                pCtx=nullptr;
            }
            return QString(tr("File broken"));
        }
        mins = secs / 60;
        secs %= 60;
        hours = mins / 60;
        mins %= 60;
        // KLOG_DEBUG() << "hh:mm:ss: " << hours << ":" << mins << ":" << secs;
    }else{
        // #62390 没有时长的视频显示为 "未知"
        if (pCtx){
            avformat_close_input(&pCtx);
            pCtx=nullptr;
        }
        return QString(tr("Unknown"));
    }

    if (pCtx){
        avformat_close_input(&pCtx);
        pCtx=nullptr;
    }
    // 目前假设单个视频文件超过99小时
    if (hours > 99){
        return QString("%1").arg(tr(">100 hours"));
    }
    return QString("%1:%2:%3").arg(hours,2,10,QLatin1Char('0')).arg(mins,2,10,QLatin1Char('0')).arg(secs,2,10,QLatin1Char('0'));
}

void Widget::createList(){
    QList<QString> headers;
    headers << tr("VideoName") << tr("Duration") << tr("Size") << tr("Date") << tr("Operation");
    for (int i = 0; i < headers.size(); i++)
    {
        QStandardItem *Item = new QStandardItem(headers.at(i));
        Item->setTextAlignment(Qt::AlignLeft);
        Item->setFont( QFont("Sans Serif", 10, QFont::Bold) );
        m_model->setHorizontalHeaderItem(i, Item);
    }

    // 导入表，固定表头
    ui->videoList->setModel(m_model);
    ui->videoList->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);

    // 无法选择
    // ui->front_list->setSelectionMode(QAbstractItemView::NoSelection);
    // 设置鼠标追踪
    ui->videoList->setMouseTracking(true);
    // 整行选择
    ui->videoList->setSelectionBehavior(QAbstractItemView::SelectRows);
    // 隐藏竖行表头
    ui->videoList->verticalHeader()->hide();
    // 不能编辑
    ui->videoList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    // 取消焦点
    ui->videoList->setFocusPolicy(Qt::NoFocus);
    // 关闭格子
    ui->videoList->setShowGrid(false);
    // 设定每行高度
    ui->videoList->verticalHeader()->setDefaultSectionSize(36);

    ui->videoList->horizontalHeader()->setFixedWidth(628);
    ui->videoList->setColumnWidth(0,202);
    ui->videoList->setColumnWidth(1,120);
    ui->videoList->setColumnWidth(2,100);
    ui->videoList->setColumnWidth(3,155);
    ui->videoList->setColumnWidth(4,70);

    // 横向滚动条禁用
//    ui->videoList->horizontalScrollBar()->setDisabled(true);
    ui->videoList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // 竖向滚动条隐藏
    ui->videoList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

}

void Widget::refreshList(QString regName)
{
    int modelRowNum = m_model->rowCount();
    if (modelRowNum > 0){
        m_model->removeRows(0,modelRowNum);
    }

    // 添加内容
    m_fileList = getVideos(ui->pathLabel->text(), regName);
    for (int p=0,i=0; p < m_fileList.size(); ++p){
        QString fileName = m_fileList.at(p).fileName();
        QString duration = getVideoDuration(m_fileList.at(p).absoluteFilePath());
        if (QString::compare(duration,tr("File broken")) == 0){
            // 录制中的视频和其他原因打不开的视频不展示 #59083
            continue;
        }
        qint64 sizeInt = m_fileList.at(p).size();
        QString fileSize = QString("%1").arg("0");
        if (sizeInt > 1073741824){
            fileSize = QString("%1GB").arg(((double)sizeInt / 1073741824),0,'f',1);
        }else if (sizeInt > 1048576){
            fileSize = QString("%1MB").arg(((double)sizeInt / 1048576),0,'f',1);
        }else {
            fileSize = QString("%1KB").arg(((double)sizeInt / 1024),0,'f',1);
        }

        QDateTime dateTime = m_fileList.at(p).lastModified();
        QString modifyDate = dateTime.toString("yyyy/MM/dd hh:mm");
        QStandardItem *fileNameItem = new QStandardItem(fileName);
        fileNameItem->setToolTip(fileName);
        m_model->setItem(i, 0, fileNameItem);
        m_model->item(i,0)->setFont( QFont("Sans Serif", 10) );
//        ui->videoList->setIndexWidget(m_model->index(p,0), createVideoNameEdit(fileName));
        m_model->setItem(i, 1, new QStandardItem());
        ui->videoList->setIndexWidget(m_model->index(i,1), createVideoDurationLabel(duration));
        m_model->setItem(i, 2, new QStandardItem(fileSize));
        m_model->item(i,2)->setFont( QFont("Sans Serif", 10) );
        m_model->setItem(i, 3, new QStandardItem(modifyDate));
        m_model->item(i,3)->setFont( QFont("Sans Serif", 10) );
        m_model->setItem(i, 4, new QStandardItem());
        ui->videoList->setIndexWidget(m_model->index(i,4), createOperationBtn(p,i));
//        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        i++;
        QCoreApplication::processEvents();
    }

}

QLineEdit *Widget::createVideoNameEdit(QString fileName)
{
    // 暂时不用该方法创建列表，文本过长显示有问题
    QLineEdit *fileNameEdit = new QLineEdit();
    fileNameEdit->setText(fileName);
    fileNameEdit->setStyleSheet("QLineEdit{"
                                "background-color:transparent;"
                                "color:#fff;"
                                "font:12px;"
                                "}");
    fileNameEdit->setEnabled(false);
    return fileNameEdit;
}

QLabel *Widget::createVideoDurationLabel(QString duration)
{
    QLabel *durationLabel = new QLabel();
    durationLabel->setText(duration);
    durationLabel->setFont(QFont("Sans Serif", 10));
    durationLabel->setStyleSheet("color:#fff;");
    return durationLabel;
}

void Widget::readConfig()
{
    QString value = m_dbusInterface->GetAuditInfo();
    QJsonParseError jError;
    QJsonDocument jsonDocument = QJsonDocument::fromJson(value.toUtf8(), &jError);
    if (jsonDocument.isObject())
    {
        m_configure = jsonDocument.object();
        return;
    }

    //判断是否因为中文utf8导致解析失败
    KLOG_INFO() << "parse json" << jsonDocument << "err, err info:" << jError.error;
    if (QJsonParseError::ParseError::IllegalUTF8String != jError.error)
        return;

    //QJsonDocument::fromJson解析中文utf8失败处理
    if (!value.startsWith("{") || !value.endsWith("}"))
        return;

    QJsonObject jsonObj;
    QStringList jsonList = value.split(",");
    for (auto jsonString : jsonList)
    {
        QStringList dataList = jsonString.split("\"");
        int index = dataList.indexOf(":");
        if (index > 0 && index < dataList.size() - 1)
        {
            jsonObj[dataList[index-1]] = dataList[index+1];
        }
    }

    m_configure = jsonObj;
}

void Widget::setConfigtoUi()
{
    // 从配置中心拿来的配置更新到ui
    for (auto k : m_configure.keys())
    {
        if (GENEARL_CONFIG_FILEPATH == k){
            QString filePath = m_configure[k].toString();
            QString homePath = QDir::homePath();
            if (filePath.startsWith("~")){
                filePath.replace(0,1,homePath);
            }
            ui->pathLabel->setText(filePath);
        }else if(GENEARL_CONFIG_FILETYPE == k){
            int setValue = UI_INDEX_LEVEL_0;
            if (QString(UI_FILETYPE_MP4) == m_configure[k].toString()){
                setValue = UI_INDEX_LEVEL_1;
            }
            ui->typeBox->setCurrentIndex(setValue);
        }else if(GENEARL_CONFIG_FPS == k){
            int ind = m_fpsList.indexOf(m_configure[k].toString());
            if (ind == -1) {
                // 手动改配置文件fps列表没有的话 设回默认值25
                ind = 3;
                QJsonObject obj;
                obj[GENEARL_CONFIG_FPS] = m_fpsList.at(ind);
                QJsonDocument doc(obj);
                QString a = QString::fromUtf8(doc.toJson(QJsonDocument::Compact).constData());
                m_dbusInterface->SetAuditItemValue(a);
            }
            ui->fpsBox->setCurrentIndex(ind);
        }else if(GENEARL_CONFIG_QUALITY == k){
            ui->clarityBox->setCurrentIndex(m_configure[k].toString().toInt());
        }else if(GENEARL_CONFIG_TIMING_PAUSE == k){
            // 无操作暂停录屏
            int setIndex = UI_INDEX_LEVEL_0;
            if (TIMING_PAUSE_LEVEL_1 == m_configure[k].toString().toInt()){
                setIndex = UI_INDEX_LEVEL_1;
            }else if (TIMING_PAUSE_LEVEL_2 == m_configure[k].toString().toInt()){
                setIndex = UI_INDEX_LEVEL_2;
            }else if (TIMING_PAUSE_LEVEL_3 == m_configure[k].toString().toInt()){
                setIndex = UI_INDEX_LEVEL_3;
            }else {
                setIndex = UI_INDEX_LEVEL_0;
            }
            ui->pauseBox->setCurrentIndex(setIndex);
        }else if (GENEARL_CONFIG_MIN_FREE_SPACE == k){
            // 磁盘监控
            int setIndex = UI_INDEX_LEVEL_3;
            if (m_configure[k].toString().toULongLong() >= AUDIT_DISK_LEVEL_3){
                setIndex = UI_INDEX_LEVEL_3;
            }else if (m_configure[k].toString().toULongLong() >= AUDIT_DISK_LEVEL_2){
                setIndex = UI_INDEX_LEVEL_2;
            }else if (m_configure[k].toString().toULongLong() >= AUDIT_DISK_LEVEL_1){
                setIndex = UI_INDEX_LEVEL_1;
            }else if (m_configure[k].toString().toULongLong() >= AUDIT_DISK_LEVEL_0){
                setIndex = UI_INDEX_LEVEL_0;
            }else {
                KLOG_DEBUG() << "disk size:" << m_configure[k].toString().toULongLong();
            }
            ui->freeSpaceBox->setCurrentIndex(setIndex);
        }else if (GENEARL_CONFIG_MAX_SAVE_DAYS == k){
            // 保存时长
            int setIndex = UI_INDEX_LEVEL_0;
            if (SAVE_MAX_DAYS_LEVEL_1 == m_configure[k].toString().toInt()){
                setIndex = UI_INDEX_LEVEL_1;
            }else if (SAVE_MAX_DAYS_LEVEL_2 == m_configure[k].toString().toInt()){
                setIndex = UI_INDEX_LEVEL_2;
            }else if (SAVE_MAX_DAYS_LEVEL_3 == m_configure[k].toString().toInt()){
                setIndex = UI_INDEX_LEVEL_3;
            }else{
                setIndex = UI_INDEX_LEVEL_0;
            }
            ui->keepTimeBox->setCurrentIndex(setIndex);
        }else if (GENEARL_CONFIG_MAX_RECORD_PER_USER == k){
            // 单个用户最大录屏数量
            int setIndex = UI_INDEX_LEVEL_0;
            if (MAX_RECORD_PER_USER_LEVEL_1 == m_configure[k].toString().toInt()){
                setIndex = UI_INDEX_LEVEL_1;
            }else if (MAX_RECORD_PER_USER_LEVEL_2 == m_configure[k].toString().toInt()){
                setIndex = UI_INDEX_LEVEL_2;
            }else if (MAX_RECORD_PER_USER_LEVEL_3 == m_configure[k].toString().toInt()){
                setIndex = UI_INDEX_LEVEL_3;
            }else{
                setIndex = UI_INDEX_LEVEL_0;
            }
            ui->maxRecordBox->setCurrentIndex(setIndex);
        } else {
            KLOG_DEBUG() << "item:" << k;
        }
    }
}

void Widget::setConfig()
{
    QJsonDocument doc(m_toSetConf);
    QString a = QString::fromUtf8(doc.toJson(QJsonDocument::Compact).constData());
    m_dbusInterface->SetAuditItemValue(a);
    KLOG_DEBUG() << __func__ << "keys to set: " << m_toSetConf.keys();

    // 清除临时列表，重新获取一下配置
    readConfig();
    for (auto k : m_toSetConf.keys()){
        m_toSetConf.remove(k);
    }

    ui->acceptBtn->hide();
    ui->cancelBtn->hide();
}

void Widget::resetConfig()
{
    // 槽函数调用
    setConfigtoUi();
    // 取消也需要清空列表
    for (auto k : m_toSetConf.keys()){
        m_toSetConf.remove(k);
    }

    ui->acceptBtn->hide();
    ui->cancelBtn->hide();
}

void Widget::testConfig(QString key, QString value){
    if (m_configure[key].toString() != value){
        m_toSetConf[key] = value;
    }else{
        m_toSetConf.remove(key);
    }

    if (m_toSetConf.length() <= 0){
        ui->acceptBtn->hide();
        ui->cancelBtn->hide();
    }else{
        ui->acceptBtn->show();
        ui->cancelBtn->show();
    }
}

void Widget::realClose()
{
    this->close();
}

void Widget::show_widget_page(QJsonObject arg)
{
    KLOG_DEBUG() << "[show widget arg]:" << arg["user"].toString() << arg["passwd"].toString();
    m_currentUserInfo = arg;
    QString currentUserName = m_currentUserInfo["user"].toString();
    // 审计管理员界面需要先展示界面再刷新列表，不然会阻塞界面
    this->show();
    if (UI_AUDADM_USERNAME == currentUserName){
        initAudadmUi();
        ui->userLevelEdit->setText(tr("Audadm"));
    }else if(UI_SYSADM_USERNAME == currentUserName){
        initSysadmUi();
        ui->userLevelEdit->setText(tr("Sysadm"));
    }else if(UI_SECADM_USERNAME == currentUserName){
        initSecadmUi();
        ui->userLevelEdit->setText(tr("Secadm"));
    }else{
        KLOG_DEBUG() << "currentUserName:" << currentUserName;
    }

}

QString Widget::strTobase64(QString inputStr)
{
    QByteArray byteArr(inputStr.toStdString().data());
    return byteArr.toBase64();
}

QString Widget::base64ToStr(QString inputStr)
{
    return QByteArray::fromBase64(QVariant(inputStr).toByteArray()).toStdString().data();
}

void Widget::getActivationInfo()
{
    ui->label_23->setText(m_activatedPage->getExpireDate());
}

void Widget::on_aboutInfoBtn_clicked()
{
    m_activatedPage->setFocus();
    m_activatedPage->exec();
}
