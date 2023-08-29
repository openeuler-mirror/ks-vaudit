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
    m_dbusInterface = new  ConfigureInterface(KSVAUDIT_CONFIGURE_SERVICE_NAME, KSVAUDIT_CONFIGURE_PATH_NAME, QDBusConnection::systemBus(), this);
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, true);
    m_activatedPage = new ActivatePage(this);
    getActivationInfo();
    this->init_ui();
    connect(ui->acceptBtn,SIGNAL(clicked()),this,SLOT(setConfig()));
    connect(ui->cancelBtn,SIGNAL(clicked()),this,SLOT(resetConfig()));

}

Widget::~Widget()
{
    delete ui;
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
    ui->acceptBtn->hide();
    ui->cancelBtn->hide();

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
    m_playAction = m_rightMenu->addAction(tr("播放"));
    m_folderAction = m_rightMenu->addAction(tr("文件位置"));

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
    m_intValidator = new QIntValidator(2, 60, this);
    ui->fpsEdit->setValidator(m_intValidator);
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
    Dialog *confirm = new Dialog(this, QString("exit"));
    connect(confirm, SIGNAL(close_window()), this, SLOT(realClose()));
    confirm->exec();
}


void Widget::on_ListBtn_clicked()
{
    if (m_currentUserInfo["user"].toString() != "audadm"){
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
    refreshList();
}

void Widget::on_ConfigBtn_clicked()
{
    if (m_currentUserInfo["user"].toString() != "sysadm"){
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
    QString dirPath = NULL;
    QString homePath = QDir::homePath();
    QString openDir = ui->pathLabel->text();
    if (openDir.startsWith("~")){
        openDir.replace(0,1,homePath);
    }
    dirPath = QFileDialog::getExistingDirectory(this, "请选择文件路径...", openDir, QFileDialog::ShowDirsOnly);
    if (dirPath != NULL){
        ui->pathLabel->setText(dirPath);
        ui->pathLabel->setToolTip(dirPath);
        refreshList(m_regName);
    }else{
        KLOG_DEBUG("Invalid path, abort!");
    }

}

void Widget::onTableBtnClicked()
{
    // 从属性里获取index
    int a = sender()->property("S_INDEX").toInt();

    // 选择行
    ui->videoList->selectRow(a);

    // 获取视频
    QList<QFileInfo>* testList = getVideos(ui->pathLabel->text(),m_regName);
    m_folderAction->setProperty("S_DIR", testList->at(a).absolutePath());
    m_playAction->setProperty("S_FILEPATH", testList->at(a).filePath());

    m_rightMenu->popup(QCursor::pos());
}


void Widget::on_minimize_clicked()
{
    Widget::showMinimized();
}

void Widget::on_fpsEdit_textChanged(const QString &arg1)
{
    QString a = arg1;
    if (a == "1"){
        // 1开头的时候边框置红，不能开始录屏
        ui->fpsEdit->setStyleSheet("QLineEdit#fpsEdit{"
                                   "background-color:#222222;"
                                   "border:1px solid #fa4949;"
                                   "border-radius:6px;"
                                   "color:#fff;"
                                   "padding-left:10px;"
                                   "}"
                                   "QLineEdit#fpsEdit:hover{"
                                   "background-color:#222222;"
                                   "border:1px solid #2eb3ff;"
                                   "border-radius:6px;"
                                   "color:#fff;"
                                   "padding-left:10px;"
                                   "}"
                                   "QLineEdit#fpsEdit:disabled{"
                                   "background-color:#393939;"
                                   "border-radius:6px;"
                                   "color:#919191;"
                                   "padding-left:10px;"
                                   "}");
        ui->label_11->setStyleSheet("color:#fa4949");
    }else if (a.startsWith("0") || a.length() == 0){
        // 有种情况是0000000开头，强行设为最低2
        ui->fpsEdit->setText("2");
        ui->fpsEdit->setStyleSheet("background-color:#222222;"
                                   "border:1px solid #393939;"
                                   "border-radius:6px;"
                                   "color:#fff;"
                                   "padding-left:10px;");
        ui->label_11->setStyleSheet("color:#999999");
    }else{
        ui->fpsEdit->setStyleSheet("QLineEdit#fpsEdit{"
                                   "background-color:#222222;"
                                   "border:1px solid #393939;"
                                   "border-radius:6px;"
                                   "color:#fff;"
                                   "padding-left:10px;"
                                   "}"
                                   "QLineEdit#fpsEdit:hover{"
                                   "background-color:#222222;"
                                   "border:1px solid #2eb3ff;"
                                   "border-radius:6px;"
                                   "color:#fff;"
                                   "padding-left:10px;"
                                   "}"
                                   "QLineEdit#fpsEdit:disabled{"
                                   "background-color:#393939;"
                                   "border-radius:6px;"
                                   "color:#919191;"
                                   "padding-left:10px;"
                                   "}");
        ui->label_11->setStyleSheet("color:#999999");
        testConfig("Fps",a);
    }
}

void Widget::on_pushButton_2_clicked()
{
    int nowFPS = ui->fpsEdit->text().toInt();
    if (nowFPS >= 60){
        return;
    }
    ui->fpsEdit->setText(QString("%1").arg(nowFPS+1));
}

void Widget::on_pushButton_3_clicked()
{
    int nowFPS = ui->fpsEdit->text().toInt();
    if (nowFPS <= 2){
        return;
    }
    ui->fpsEdit->setText(QString("%1").arg(nowFPS-1));
}

void Widget::on_fpsEdit_returnPressed()
{
    if (ui->fpsEdit->hasFocus()){
        ui->fpsEdit->clearFocus();
    }
}

void Widget::on_searchBar_editingFinished()
{
    m_regName = ui->searchBar->text();
    refreshList(m_regName);
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
    testConfig("Quality", QString("%1").arg(index));
}

void Widget::on_pauseBox_currentIndexChanged(int index)
{
    int setValue = 0;
    if (index == 0){
        setValue = 0;
    }else if (index == 1){
        setValue = 5;
    }else if (index == 2){
        setValue = 10;
    }else if (index == 3){
        setValue = 15;
    }
    testConfig(QString("TimingPause"), QString("%1").arg(setValue));
}

void Widget::on_typeBox_currentIndexChanged(int index)
{
    QString setValue = QString("MP4");
    if (index == 0){
        setValue = QString("MP4");
    }else if (index = 1){
        setValue = QString("OGV");
    }
    testConfig(QString("FileType"), setValue);
}

void Widget::on_freeSpaceBox_currentIndexChanged(int index)
{
    unsigned long long setValue = 1073741824;
    if (index == 0){
        setValue *= 5;
    }else if (index == 1){
        setValue *= 10;
    }else if (index == 2){
        setValue *= 15;
    }else if (index == 3){
        setValue *= 20;
    }
    testConfig(QString("MinFreeSpace"), QString("%1").arg(setValue));
}

void Widget::on_keepTimeBox_currentIndexChanged(int index)
{
    int setValue = 3;
    if (index == 0){
        setValue = 3;
    }else if (index == 1){
        setValue = 7;
    }else if (index == 2){
        setValue = 15;
    }else if (index == 3){
        setValue = 30;
    }
    testConfig(QString("MaxSaveDays"), QString("%1").arg(setValue));
}

void Widget::on_maxRecordBox_currentIndexChanged(int index)
{
    int setValue = 0;
    if (index == 0){
        setValue = 0;
    }else if (index == 1){
        setValue = 1;
    }else if (index == 2){
        setValue = 3;
    }else if (index == 3){
        setValue = 5;
    }
    testConfig(QString("MaxRecordPerUser"), QString("%1").arg(setValue));
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
        retString = "密码长度需要在8-30位之间";
    }else if(inputpw.contains(QRegExp("[\\x4e00-\\x9fa5]+"))){
        retString = "密码不能包含中文字符";
    }else if(!inputpw.contains(charReg)){
        retString = "密码必须包含一位特殊字符";
    }else if(!inputpw.contains(QRegExp("[0-9]+"))){
        retString = "密码必须包含一位数字";
    }else if(!inputpw.contains(QRegExp("[a-zA-Z]+"))){
        retString = "密码必须包含一个字母";
    }

    return retString;
}

void Widget::playVideo()
{
    QString filePath = sender()->property("S_FILEPATH").toString();
    QProcess *p = new QProcess(this);
    p->setProcessChannelMode(QProcess::MergedChannels);
    QStringList commands;
    commands << filePath;
    // 采用分离式启动(startDetached)ffplay，因为和本录屏软件无关
    p->startDetached("ffplay", commands);
}

void Widget::openDir()
{
    QString videoPath = sender()->property("S_DIR").toString();
    QDesktopServices::openUrl(QUrl::fromLocalFile(videoPath));
}

QPushButton *Widget::createOperationBtn(int index)
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
    aBtn->setProperty("S_INDEX", index);

    connect(aBtn, SIGNAL(clicked(bool)), this, SLOT(onTableBtnClicked()));
    return aBtn;
}

QList<QFileInfo>* Widget::getVideos(QString path, QString regName = QString(""))
{
    if (path.startsWith("~")){
        path = path.replace(0,1,QDir::homePath());
    }
    QDir *dir = new QDir(path);
    QStringList filters;
    filters << "*.mp4" << "*.ogv";
    dir->setNameFilters(filters);
    // 默认时间排序
    dir->setSorting(QDir::Time);
    dir->setFilter(QDir::Files | QDir::NoSymLinks);
    QList<QFileInfo> *filesInfo = new QList<QFileInfo>(dir->entryInfoList());
    QList<QFileInfo> *filesInfoRet = new QList<QFileInfo>();
    for (int i = 0; i < filesInfo->size(); ++i){
        if (regName.isEmpty()){
            break;
        }
        if (filesInfo->at(i).fileName().contains(regName)){
            filesInfoRet->append(filesInfo->at(i));
        }
    }
    return regName.isEmpty() ? filesInfo : filesInfoRet;
}

QString Widget::getVideoDuration(QString absPath)
{
    int secs = 0;
    int mins = 0;
    int hours = 0;
    AVFormatContext* pCtx = NULL;
    int ret = avformat_open_input(&pCtx, absPath.toStdString().c_str(), NULL, NULL);
    if (ret < 0){
        KLOG_DEBUG() << "avformat_open_input() failed:" << ret;
        return QString("文件损坏");
    }
    int ret1 = avformat_find_stream_info(pCtx,NULL);
    if(ret1 < 0){
        KLOG_DEBUG() << "avformat_find_stream_info() failed:" << ret1;
        return QString("文件损坏");
    }

    if (pCtx->duration != AV_NOPTS_VALUE){
        int64_t duration = pCtx->duration + (pCtx->duration <= INT64_MAX - 5000 ? 5000 : 0);
        secs = duration / AV_TIME_BASE;
        mins = secs / 60;
        secs %= 60;
        hours = mins / 60;
        mins %= 60;
        // KLOG_DEBUG() << "hh:mm:ss: " << hours << ":" << mins << ":" << secs;
    }else{
        KLOG_DEBUG() << absPath << "has no duration";
    }

    if (pCtx != NULL){
        avformat_close_input(&pCtx);
        pCtx=NULL;
    }
    // 目前假设单个视频文件超过99小时
    if (hours > 99){
        return QString("%1").arg("大于99小时");
    }
    return QString("%1:%2:%3").arg(hours,2,10,QLatin1Char('0')).arg(mins,2,10,QLatin1Char('0')).arg(secs,2,10,QLatin1Char('0'));
}

void Widget::createList(){
    QList<QString> headers;
    headers << "视频名" << "时长" << "大小" << "日期"<<"操作";
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
    ui->videoList->setColumnWidth(1,108);
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
    QList<QFileInfo>* testList;
    testList = getVideos(ui->pathLabel->text(), regName);
    m_fileList = testList;

    for (int p,i = 0; p < testList->size(); ++p){
        QString fileName = testList->at(p).fileName();
        QString duration = getVideoDuration(testList->at(p).absoluteFilePath());
        if (QString::compare(duration,"文件损坏") == 0){
            // 录制中的视频和其他原因打不开的视频不展示 #59083
            continue;
        }
        qint64 sizeInt = testList->at(p).size();
        QString fileSize = QString("%1").arg("0");
        if (sizeInt > 1073741824){
            fileSize = QString("%1GB").arg(((double)sizeInt / 1073741824),0,'f',1);
        }else if (sizeInt > 1048576){
            fileSize = QString("%1MB").arg(((double)sizeInt / 1048576),0,'f',1);
        }else {
            fileSize = QString("%1KB").arg(((double)sizeInt / 1024),0,'f',1);
        }

        QDateTime dateTime = testList->at(p).lastModified();
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
        ui->videoList->setIndexWidget(m_model->index(i,4), createOperationBtn(i));
//        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        QCoreApplication::processEvents();
        i++;
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
    QJsonDocument doc = QJsonDocument::fromJson(value.toUtf8());
    m_configure = doc.isObject() ? doc.object() : m_configure;
}

void Widget::setConfigtoUi()
{
    // 从配置中心拿来的配置更新到ui
    for (auto k : m_configure.keys())
    {
        if (k == "FilePath"){
            QString filePath = m_configure[k].toString();
            QString homePath = QDir::homePath();
            if (filePath.startsWith("~")){
                filePath.replace(0,1,homePath);
            }
            ui->pathLabel->setText(filePath);
        }else if(k == "FileType"){
            int setValue = 0;
            if (m_configure[k].toString() == QString("OGV")){
                setValue = 1;
            }
            ui->typeBox->setCurrentIndex(setValue);
        }else if(k == "Fps"){
            ui->fpsEdit->setText(m_configure[k].toString());
        }else if(k == "Quality"){
            ui->clarityBox->setCurrentIndex(m_configure[k].toString().toInt());
        }else if(k == "TimingPause"){
            // 无操作暂停录屏
            int setIndex = 0;
            if (m_configure[k].toString().toInt() == 0){
                setIndex = 0;
            }else if (m_configure[k].toString().toInt() == 5){
                setIndex = 1;
            }else if (m_configure[k].toString().toInt() == 10){
                setIndex = 2;
            }else if (m_configure[k].toString().toInt() == 30){
                setIndex = 3;
            }
            ui->pauseBox->setCurrentIndex(setIndex);
        }else if (k == "MinFreeSpace"){
            // 磁盘监控
            int setIndex = 3;
            if (m_configure[k].toString().toULongLong() >= 21474836480){
                setIndex = 3;
            }else if (m_configure[k].toString().toULongLong() >= 16106127360){
                setIndex = 2;
            }else if (m_configure[k].toString().toULongLong() >= 10737418240){
                setIndex = 1;
            }else if (m_configure[k].toString().toULongLong() >= 5368709120){
                setIndex = 0;
            }
            ui->freeSpaceBox->setCurrentIndex(setIndex);
        }else if (k == "MaxSaveDays"){
            // 保存时长
            int setIndex = 0;
            if (m_configure[k].toString().toInt() == 3){
                setIndex = 0;
            }else if (m_configure[k].toString().toInt() == 7){
                setIndex = 1;
            }else if (m_configure[k].toString().toInt() == 15){
                setIndex = 2;
            }else if (m_configure[k].toString().toInt() == 30){
                setIndex = 3;
            }
            ui->keepTimeBox->setCurrentIndex(setIndex);
        }else if (k == "MaxRecordPerUser"){
            // 单个用户最大录屏数量
            int setIndex = 0;
            if (m_configure[k].toString().toInt() == 0){
                setIndex = 0;
            }else if (m_configure[k].toString().toInt() == 1){
                setIndex = 1;
            }else if (m_configure[k].toString().toInt() == 3){
                setIndex = 2;
            }else if (m_configure[k].toString().toInt() == 5){
                setIndex = 3;
            }
            ui->maxRecordBox->setCurrentIndex(setIndex);
        }
    }
}

void Widget::setConfig()
{
    if (ui->fpsEdit->text() == "1"){
        return;
    }
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
    if (currentUserName == "audadm"){
        initAudadmUi();
        ui->userLevelEdit->setText("审计管理员");
    }else if(currentUserName == "sysadm"){
        initSysadmUi();
        ui->userLevelEdit->setText("系统管理员");
    }else if(currentUserName == "secadm"){
        initSecadmUi();
        ui->userLevelEdit->setText("安全管理员");
    }

    this->show();
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
    m_activatedPage->exec();
}
