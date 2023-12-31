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
#include <QJsonDocument>
#include <QJsonObject>
#include "ksvaudit-configure_global.h"
#include "common-definition.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/pixdesc.h"
#include "libavutil/pixfmt.h"
#include "libavutil/dict.h"
#include <pwd.h>
}

#define SLIDER_DELAY_MS 100
#define HEARTBEAT_MS 3000

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
//    setAutoFillBackground(false);
    setWindowFlags(Qt::FramelessWindowHint);
//    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);

    m_dbusInterface = new ConfigureInterface(KSVAUDIT_CONFIGURE_SERVICE_NAME, KSVAUDIT_CONFIGURE_PATH_NAME, QDBusConnection::systemBus(), this);
    m_activatePage = new ActivatePage();
    m_isActivated = m_activatePage->getActivation();
    if (!m_isActivated){
        m_activatePage->setFocus();
        m_activatePage->exec();
    }
    // 音量调节定时器
    m_sendMData = new QTimer();
    m_sendSData = new QTimer();
    m_sendMData->setSingleShot(true);
    m_sendSData->setSingleShot(true);
    connect(m_sendMData,SIGNAL(timeout()),this,SLOT(sendMicToConfig()));
    connect(m_sendSData,SIGNAL(timeout()),this,SLOT(sendSpkToConfig()));
    // 心跳检测定时器
    m_heartBeat = new QTimer();
    connect(m_heartBeat,SIGNAL(timeout()),this,SLOT(reconnectMonitor()));
    init_ui();
}

Widget::~Widget()
{
    delete ui;
    sendSwitchControl(m_selfPID, m_recordPID, OPERATE_RECORD_EXIT);
    if (m_dbusInterface){
        delete m_dbusInterface;
        m_dbusInterface = nullptr;
    }
    if (m_model){
        m_model->clear();
        delete m_model;
        m_model = nullptr;
    }
    if (m_sendMData) {
        delete m_sendMData;
        m_sendMData = nullptr;
    }
    if (m_sendSData) {
        delete m_sendSData;
        m_sendSData = nullptr;
    }
    if (m_heartBeat) {
        delete m_heartBeat;
        m_heartBeat = nullptr;
    }
}


void Widget::init_ui()
{
    ui->NormalBody->show();
    ui->NormalFooterBar->show();
    ui->ListBody->hide();
    ui->listArrow->hide();
    ui->configArrow->hide();
    ui->ConfigBody->hide();
    ui->stopBtn->setDisabled(true);
    comboboxStyle();

    if (m_fpsList.isEmpty()){
        m_fpsList << "5" << "10" << "20" << "25" << "30" << "45" << "60";
    }
    ui->waterprintText->setMaxLength(20);
    QMenu* moreMenu = new QMenu(this);
    moreMenu->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
    moreMenu->setAttribute(Qt::WA_TranslucentBackground);
//    moreMenu->setObjectName("moreMenu");
    QAction* activateAction = moreMenu->addAction(tr("Software Activate"));
    QAction* aboutAction = moreMenu->addAction(tr("About Software"));
    connect(aboutAction, SIGNAL(triggered()), this, SLOT(openAbout()));
    connect(activateAction, SIGNAL(triggered()), this, SLOT(openActivate()));

    ui->moreMenuBtn->setMenu(moreMenu);

    // 列表右键弹出菜单
    m_rightMenu = new QMenu(this);
    m_rightMenu->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
    m_rightMenu->setAttribute(Qt::WA_TranslucentBackground);
    m_rightMenu->setObjectName("listMenu");
    m_playAction = m_rightMenu->addAction(tr("Play"));
    m_renameAction = m_rightMenu->addAction(tr("Rename"));
    m_folderAction = m_rightMenu->addAction(tr("File Location"));
    m_deleteAction = m_rightMenu->addAction(tr("Delete"));

    connect(m_playAction, SIGNAL(triggered()), this, SLOT(playVideo()));
    connect(m_renameAction, SIGNAL(triggered()), this, SLOT(renameVideo()));
    connect(m_folderAction, SIGNAL(triggered()), this, SLOT(openDir()));
    connect(m_deleteAction, SIGNAL(triggered()), this, SLOT(deleteVideo()));
    connect(m_dbusInterface, SIGNAL(SignalSwitchControl(int,int,QString)), this, SLOT(refreshTime(int, int, QString)));
    connect(m_dbusInterface, SIGNAL(SignalNotification(int, QString)), this, SLOT(receiveNotification(int, QString)));

    readConfig();
    // 构建模型表头
    m_model = new QStandardItemModel();
    createList();
    refreshList(m_regName);
    m_selfPID = QCoreApplication::applicationPid();
    // 向monitor发送启后台录屏进程信息
    QString args = QString("process=%1;DISPLAY=%2;XAUTHORITY=%3;USER=%4;HOME=%5").arg(m_selfPID).arg(getenv("DISPLAY")).arg(getenv("XAUTHORITY")).arg(getenv("USER")).arg(getenv("HOME"));
    sendSwitchControl(m_selfPID, 0, args);
}

void Widget::comboboxStyle(){
    ui->clarityBox->setView(new QListView());
    ui->fpsBox->setView(new QListView());
    ui->remainderBox->setView(new QListView());
    ui->typeBox->setView(new QListView());
    ui->resolutionBox->setView(new QListView());
    ui->audioBox->setView(new QListView());
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
    m_pConfirm = new Dialog(this, QString("exit"));
    connect(m_pConfirm, SIGNAL(close_window()), this, SLOT(realClose()));
    m_pConfirm->exec();
}

void Widget::on_NormalBtn_clicked()
{
    ui->ListBody->hide();
    ui->listArrow->hide();
    ui->NormalBody->show();
    ui->NormalFooterBar->show();
    ui->normalArrow->show();
    ui->configArrow->hide();
    ui->ConfigBody->hide();
    ui->NormalBtn->setStyleSheet("QPushButton#NormalBtn{"
                        "border: 0px;"
                        "background-color:#2eb3ff;"
                        "color: rgb(255, 255, 255);"
                        "border-radius: 4px;"
                        "text-align:left center;"
                        "padding-left:10px;"
                        "};");
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
}

void Widget::on_ListBtn_clicked()
{
    ui->NormalBody->hide();
    ui->normalArrow->hide();
    ui->NormalFooterBar->hide();
    ui->ListBody->show();
    ui->listArrow->show();
    ui->configArrow->hide();
    ui->ConfigBody->hide();
    ui->ListBtn->setStyleSheet("QPushButton#ListBtn{"
                               "border: 0px;"
                               "background-color:#2eb3ff;"
                               "color: rgb(255, 255, 255);"
                               "border-radius: 6px;"
                               "text-align:left center;"
                               "padding-left:10px;"
                               "};");
    ui->NormalBtn->setStyleSheet("QPushButton#NormalBtn{"
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
    refreshList(m_regName);

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
        if (dirPath.startsWith(homePath)){
            dirPath.replace(0,homePath.size(),"~");
        }
        ui->pathLabel->setText(dirPath);
        ui->pathLabel->setToolTip(dirPath);
        if (dirPath.startsWith("~")){
            dirPath = dirPath.replace(0,1,QDir::homePath());
        }
        setConfig(GENEARL_CONFIG_FILEPATH, dirPath);
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
    m_folderAction->setProperty("S_DIR", m_fileList.at(m).absolutePath());
    m_playAction->setProperty("S_FILEPATH", m_fileList.at(m).filePath());
    m_deleteAction->setProperty("S_FILEPATH", m_fileList.at(m).filePath());
    m_renameAction->setProperty("S_OLDNAME", m_fileList.at(m).fileName());
    m_renameAction->setProperty("S_OLDPATH", m_fileList.at(m).filePath());

    m_rightMenu->popup(QCursor::pos());
}

void Widget::on_waterprintCheck_stateChanged(int arg1)
{
    QString ret;
    if (UI_INDEX_LEVEL_0 == arg1){
        ret = QString("%1").arg(arg1);
        KLOG_DEBUG("hide waterprint");
        ui->waterprintText->hide();
        ui->waterprintConfirm->hide();
        ui->label_6->hide();
    }else if (UI_INDEX_LEVEL_2 == arg1){
        // QT的checkbox勾选是2，半勾选是1。
        // 所以这里2需要改成1传到配置中心去
        ret = QString("%1").arg(1);
        KLOG_DEBUG("show waterprint");
        ui->waterprintText->show();
        ui->waterprintConfirm->show();
        ui->label_6->show();
    }else {
        KLOG_DEBUG() << "arg:" << arg1;
    }
    setConfig(QString(GENEARL_CONFIG_WATER_PRINT), ret);
}

void Widget::on_volumnBtn_clicked()
{
    if(ui->volumnSlider->value() > VOLUME_MIN_VALUE){
        ui->volumnSlider->setValue(VOLUME_MIN_VALUE);
        ui->volumnBtn->setStyleSheet("image:url(:/images/v0.svg);border:none;");
    }else if (ui->volumnSlider->value() == VOLUME_MIN_VALUE){
        ui->volumnSlider->setValue(VOLUME_MAX_VALUE);
    }else {
        KLOG_DEBUG() << "value error";
    }
}

void Widget::on_volumnSlider_valueChanged(int value)
{
    if (value == VOLUME_MIN_VALUE){
        ui->volumnBtn->setStyleSheet("image:url(:/images/v0.svg);border:none;");
    }else if (value < VOLUME_MID_VALUE){
        ui->volumnBtn->setStyleSheet("image:url(:/images/v2.svg);border:none;");
    }else if (value >= VOLUME_MID_VALUE){
        ui->volumnBtn->setStyleSheet("image:url(:/images/v1.svg);border:none;");
    }else {
        KLOG_DEBUG() << "value error";
    }
    if (m_sendSData){
        m_sendSData->start(SLIDER_DELAY_MS);
    }
}

void Widget::on_audioBtn_clicked()
{
    if(ui->audioSlider->value() > VOLUME_MIN_VALUE){
        ui->audioSlider->setValue(VOLUME_MIN_VALUE);
        ui->audioBtn->setStyleSheet("image:url(:/images/m0.svg);border:none;");
    }else if (ui->audioSlider->value() == VOLUME_MIN_VALUE){
        ui->audioSlider->setValue(VOLUME_MAX_VALUE);
    }else {
        KLOG_DEBUG() << "value error";
    }
}

void Widget::on_audioSlider_valueChanged(int value)
{
    if (value == VOLUME_MIN_VALUE){
        ui->audioBtn->setStyleSheet("image:url(:/images/m0.svg);border:none;");
    }else if (value > VOLUME_MIN_VALUE){
        ui->audioBtn->setStyleSheet("image:url(:/images/m1.svg);border:none;");
    }else {
        KLOG_DEBUG() << "value error";
    }
    if (m_sendMData){
        m_sendMData->start(SLIDER_DELAY_MS);
    }
}

void Widget::on_playBtn_clicked()
{
    m_isActivated = m_activatePage->getActivation();
    if (!m_isActivated){
        m_activatePage->setFocus();
        m_activatePage->exec();
        return;
    }
    if (m_heartBeat->isActive()){
        m_heartBeat->stop();
    }
    m_isRecording = !m_isRecording;
    if (m_isRecording){
        Widget::showMinimized();
        ui->playBtn->setStyleSheet("image:url(:/images/pauseRecord.svg);border:none;");
        ui->audioBox->setDisabled(true);
        ui->resolutionBox->setDisabled(true);
        ui->clarityBox->setDisabled(true);
        ui->fpsBox->setDisabled(true);
        ui->remainderBox->setDisabled(true);
        ui->typeBox->setDisabled(true);
        ui->pushButton->setDisabled(true);
        if (m_needRestart){
            sendSwitchControl(m_selfPID, m_recordPID, OPERATE_RECORD_RESTART);
        }else{
            sendSwitchControl(m_selfPID, m_recordPID, OPERATE_RECORD_START);
        }
        m_needRestart = false;
        m_heartBeat->start(HEARTBEAT_MS);
        KLOG_DEBUG("Start record screen!");
    }else{
        sendSwitchControl(m_selfPID, m_recordPID, OPERATE_RECORD_PAUSE);
        m_needRestart = true;
        ui->playBtn->setStyleSheet("image:url(:/images/record.svg);border:none;");
        KLOG_DEBUG("Pause record screen!");
    }
    ui->stopBtn->setDisabled(false);
}

void Widget::on_stopBtn_clicked()
{
    sendSwitchControl(m_selfPID, m_recordPID, OPERATE_RECORD_STOP);
    ui->timeStamp->setText("0:00:00");
    m_isRecording = false;
    m_needRestart = false;
    ui->playBtn->setStyleSheet("image:url(:/images/record.svg);border:none;");
    ui->audioBox->setDisabled(false);
    ui->resolutionBox->setDisabled(false);
    ui->clarityBox->setDisabled(false);
    ui->fpsBox->setDisabled(false);
    ui->remainderBox->setDisabled(false);
    ui->typeBox->setDisabled(false);
    ui->pushButton->setDisabled(false);
    ui->stopBtn->setDisabled(true);
}

void Widget::on_ConfigBtn_clicked()
{
    ui->NormalBody->hide();
    ui->normalArrow->hide();
    ui->NormalFooterBar->hide();
    ui->ListBody->hide();
    ui->listArrow->hide();
    ui->configArrow->show();
    ui->ConfigBody->show();
    ui->ConfigBtn->setStyleSheet("QPushButton#ConfigBtn{"
                               "border: 0px;"
                               "background-color:#2eb3ff;"
                               "color: rgb(255, 255, 255);"
                               "border-radius: 6px;"
                               "text-align:left center;"
                               "padding-left:10px;"
                               "};");
    ui->NormalBtn->setStyleSheet("QPushButton#NormalBtn{"
                        "border: 0px;"
                        "background-color:rgb(57,57,57);"
                        "color: rgb(255, 255, 255);"
                        "border-radius: 4px;"
                        "text-align:left center;"
                        "padding-left:10px;"
                        "};");
    ui->ListBtn->setStyleSheet("QPushButton#ListBtn{"
                                 "border: 0px;"
                                 "background-color:rgb(57,57,57);"
                                 "color: rgb(255, 255, 255);"
                                 "border-radius: 4px;"
                                 "text-align:left center;"
                                 "padding-left:10px;"
                                 "};");
}

void Widget::on_minimize_clicked()
{
    Widget::showMinimized();
}

void Widget::on_fpsBox_currentIndexChanged(int index)
{
    setConfig(QString(GENEARL_CONFIG_FPS), m_fpsList.at(index));
}

void Widget::on_waterprintText_returnPressed()
{
    if (ui->waterprintText->hasFocus()){
        ui->waterprintText->clearFocus();
    }
}

void Widget::on_searchBar_returnPressed()
{
    m_regName = ui->searchBar->text();
    refreshList(m_regName);
    if (ui->searchBar->hasFocus()){
        ui->searchBar->clearFocus();
    }
}

void Widget::on_waterprintConfirm_clicked()
{
    ui->waterprintText->clearFocus();
    if (ui->waterprintConfirm->text() == QString(tr("Modify"))){
        ui->waterprintText->setDisabled(false);
        ui->waterprintConfirm->setText(tr("Confirm"));
        KLOG_DEBUG() << __func__ << "waterprintText Revising ...";
    }else{
        ui->waterprintText->setDisabled(true);
        ui->waterprintConfirm->setText(tr("Modify"));
        m_waterText = ui->waterprintText->text();
        setConfig(QString(GENEARL_CONFIG_WATER_PRINT), QString("%1").arg(ui->waterprintCheck->isChecked()));
        setConfig(QString(GENEARL_CONFIG_WATER_PRINT_TEXT), ui->waterprintText->text());
        KLOG_DEBUG() << __func__ << "waterprintText Confirmed ...";
    }

}

void Widget::on_waterprintText_textChanged(const QString &arg1)
{
    if (m_waterText != arg1){
        ui->waterprintConfirm->setDisabled(false);
    }else{
        ui->waterprintConfirm->setDisabled(true);
    }
}

void Widget::on_resolutionBox_currentIndexChanged(int index)
{
    QVariant v(1|32);
    if (index == 0){
        v.setValue(0);
    }
    ui->audioBox->setItemData(3, v, Qt::UserRole -1 );
    setConfig(QString(GENEARL_CONFIG_RECORD_VIDIO), QString("%1").arg(index));
}

void Widget::on_audioBox_currentIndexChanged(int index)
{
    QVariant v(1|32);
    QString setValue = QString(CONFIG_RECORD_AUDIO_ALL);
    if (UI_INDEX_LEVEL_0 == index){
        setValue = QString(CONFIG_RECORD_AUDIO_ALL);
    }else if(UI_INDEX_LEVEL_1 == index){
        setValue = QString(CONFIG_RECORD_AUDIO_SPEAKER);
    }else if(UI_INDEX_LEVEL_2 == index){
        setValue = QString(CONFIG_RECORD_AUDIO_MIC);
    }else if(UI_INDEX_LEVEL_3 == index){
        setValue = QString(CONFIG_RECORD_AUDIO_NONE);
        v.setValue(0);
    }else{
        KLOG_DEBUG() << "index error:" << index;
    }
    ui->resolutionBox->setItemData(0, v, Qt::UserRole -1 );
    setConfig(QString(GENEARL_CONFIG_RECORD_AUDIO), setValue);
}

void Widget::on_clarityBox_currentIndexChanged(int index)
{
    setConfig(QString(GENEARL_CONFIG_QUALITY), QString("%1").arg(index));
}

void Widget::on_remainderBox_currentIndexChanged(int index)
{
    int setValue = TIMING_REMINDER_LEVEL_0;
    if (UI_INDEX_LEVEL_0 == index){
        setValue = TIMING_REMINDER_LEVEL_0;
    }else if (UI_INDEX_LEVEL_1 == index){
        setValue = TIMING_REMINDER_LEVEL_1;
    }else if (UI_INDEX_LEVEL_2 == index){
        setValue = TIMING_REMINDER_LEVEL_2;
    }else if (UI_INDEX_LEVEL_3 == index){
        setValue = TIMING_REMINDER_LEVEL_3;
    }else {
        KLOG_DEBUG() << "index error:" << index;
    }

    m_timing = setValue;
    setConfig(QString(GENEARL_CONFIG_TIMING_REMINDER), QString("%1").arg(setValue));
}

void Widget::on_typeBox_currentIndexChanged(int index)
{
    QString setValue = QString(UI_FILETYPE_MKV);
    if (UI_INDEX_LEVEL_0 == index){
        setValue = QString(UI_FILETYPE_MKV);
    }else if (UI_INDEX_LEVEL_1 == index){
        setValue = QString(UI_FILETYPE_MP4);
    }else {
        KLOG_DEBUG() << "index error:" << index;
    }
    setConfig(QString(GENEARL_CONFIG_FILETYPE), setValue);
}


void Widget::openAbout()
{
    Dialog *aboutDialog = new Dialog(this,QString("about"));
    aboutDialog->exec();
}

void Widget::renameVideo()
{
    QString oldName = sender()->property("S_OLDNAME").toString();

//    KLOG_DEBUG() << oldName << "left:" << oldName.left(oldName.lastIndexOf(".")) << "right:" << oldName.right(4);
    if (m_renameDialog){
//        delete m_renameDialog;
        m_renameDialog = nullptr;
    }
    m_renameDialog = new Dialog(this, QString("rename"), oldName);
    connect(m_renameDialog, SIGNAL(rename_file()), this, SLOT(realRename()));
    m_renameDialog->exec();
}

void Widget::realRename()
{
    QString oldName = m_renameDialog->getOldName();
    QString newName = m_renameDialog->getNewName();
    if (QString::compare(oldName, newName) == 0){
        Dialog warningDialog(this,"Warning");
        warningDialog.exec();
    }
    for (int i = 0; i < m_fileList.size(); ++i){
        KLOG_DEBUG() << m_fileList.at(i).fileName() << ":" << oldName;
        if (m_fileList.at(i).fileName() == oldName){
            QFile file(m_fileList.at(i).filePath());
            bool ret = file.rename(QString("%1/%2").arg(m_fileList.at(i).absolutePath()).arg(newName));
            KLOG_DEBUG() << "rename file ok: " << ret;
            m_renameDialog->close();
            refreshList();
            break;
        }
    }
}

void Widget::playVideo()
{
    QString filePath = sender()->property("S_FILEPATH").toString();
    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);
    QStringList commands;
//    commands << "-slave";
//    commands << "-quiet";
    commands << filePath;
    // 采用分离式启动(startDetached)mplayer/ffplay，因为和本录屏软件无关
    p.startDetached("ffplay", commands);
}

void Widget::openDir()
{
    QString videoPath = sender()->property("S_DIR").toString();
    QDesktopServices::openUrl(QUrl::fromLocalFile(videoPath));
}

void Widget::deleteVideo()
{
    Dialog *confirmDelete = new Dialog(this, QString("delete"));
    connect(confirmDelete, SIGNAL(delete_file()), this, SLOT(realDelete()));
    confirmDelete->exec();
}

void Widget::realDelete()
{
    QString filePath = m_deleteAction->property("S_FILEPATH").toString();
    if (QDir().exists(filePath)){
        QFileInfo fileInfo(filePath);
//        KLOG_DEBUG() << "isFile: " << fileInfo.isFile();
        QFile::remove(filePath);
        refreshList(m_regName);
    }else{
        KLOG_DEBUG() << "No such file";
    }
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
//    fileNameEdit->setMaximumWidth(302);
    fileNameEdit->setFixedSize(190,36);
    fileNameEdit->setStyleSheet("QLineEdit{"
                                "background-color:red;"
                                "color:#fff;"
                                "font:12px;"
                                "}");
    fileNameEdit->setEnabled(false);
    return fileNameEdit;
}

void Widget::readConfig()
{
    QString value = m_dbusInterface->GetRecordInfo();
    QJsonObject jsonObj;
    if (parseJsonData(value, jsonObj))
    {
        for (auto k : jsonObj.keys())
        {
//            qWarning() << __func__ << "GetRecordInfo" << k << jsonObj[k].toString();
            if (GENEARL_CONFIG_FILEPATH == k){
                QString filePath = jsonObj[k].toString();
                QString homeDir = QDir::homePath();
                // 以~开头的路径，在目录不存在时exists返回true
                // 通过monitor启后台程序，创建路径的数组为root，因路径层数导致不好修改权限，直接在前台创建目录
                if (filePath.contains('~')){
                    QString path = filePath;
                    path.replace('~', homeDir);
                    KLOG_INFO() << "QFile::exists(path): " << QFile::exists(path) << path << filePath;
                    if (!QFile::exists(path)){ //存放目录不存在时创建一个
                        QDir dir;
                        if (!dir.exists(path) && !dir.mkpath(path)){
                            KLOG_WARNING() << "can't mkdir the file path " << path;
                        }
                    }
                }

                if (filePath.startsWith(homeDir)){
                    filePath.replace(0,homeDir.size(), "~");
                }
                ui->pathLabel->setText(filePath);
            }else if(GENEARL_CONFIG_FILETYPE == k){
                int setValue = 0;
                if (QString(UI_FILETYPE_MP4) == jsonObj[k].toString()){
                    setValue = 1;
                }
                ui->typeBox->setCurrentIndex(setValue);
            }else if(GENEARL_CONFIG_FPS == k){
                int ind = m_fpsList.indexOf(jsonObj[k].toString());
                if (ind == -1) {
                    // 手动改配置文件fps列表没有的话 设回默认值25
                    ind = 3;
                    setConfig(GENEARL_CONFIG_FPS, m_fpsList.at(ind));
                }
                ui->fpsBox->setCurrentIndex(ind);
            }else if(GENEARL_CONFIG_QUALITY == k){
                ui->clarityBox->setCurrentIndex(jsonObj[k].toString().toInt());
            }else if(GENEARL_CONFIG_RECORD_AUDIO == k){
                int setIndex = UI_INDEX_LEVEL_0;
                QVariant v(1|32);
                QString auditType = jsonObj[k].toString();
                if(QString(CONFIG_RECORD_AUDIO_SPEAKER) == auditType){
                    setIndex = UI_INDEX_LEVEL_1;
                }else if(QString(CONFIG_RECORD_AUDIO_MIC) == auditType){
                    setIndex = UI_INDEX_LEVEL_2;
                }else if(QString(CONFIG_RECORD_AUDIO_NONE) == auditType){
                    setIndex = UI_INDEX_LEVEL_3;
                    v.setValue(0);
                }else{
                    setIndex = UI_INDEX_LEVEL_0;
                }

                ui->resolutionBox->setItemData(0, v, Qt::UserRole -1 );
                ui->audioBox->setCurrentIndex(setIndex);
            }else if(GENEARL_CONFIG_RECORD_VIDIO == k){
                int index = jsonObj[k].toString().toInt();
                ui->resolutionBox->setCurrentIndex(index);
                QVariant v(1|32);
                if (index == 0){
                    v.setValue(0);
                }
                ui->audioBox->setItemData(3, v, Qt::UserRole -1 );
            }else if(GENEARL_CONFIG_TIMING_REMINDER == k){
                int setIndex = UI_INDEX_LEVEL_0;
                if (TIMING_REMINDER_LEVEL_1 == jsonObj[k].toString().toInt()){
                    setIndex = UI_INDEX_LEVEL_1;
                }else if (TIMING_REMINDER_LEVEL_2 == jsonObj[k].toString().toInt()){
                    setIndex = UI_INDEX_LEVEL_2;
                }else if (TIMING_REMINDER_LEVEL_3 == jsonObj[k].toString().toInt()){
                    setIndex = UI_INDEX_LEVEL_3;
                }else{
                    setIndex = UI_INDEX_LEVEL_0;
                }
                ui->remainderBox->setCurrentIndex(setIndex);
                m_timing = jsonObj[k].toString().toInt();
            }else if(GENEARL_CONFIG_WATER_PRINT == k){
                ui->waterprintCheck->setChecked((bool)jsonObj[k].toString().toInt());
            }else if(GENEARL_CONFIG_WATER_PRINT_TEXT == k){
                ui->waterprintText->setText(jsonObj[k].toString());
                m_waterText = jsonObj[k].toString();
            }else if(GENEARL_CONFIG_MIC_VOLUME == k){
                ui->audioSlider->setValue(jsonObj[k].toString().toInt());
            }else if(GENEARL_CONFIG_SPEAKER_VOLUME == k){
                ui->volumnSlider->setValue(jsonObj[k].toString().toInt());
            }else{
                KLOG_DEBUG() << "item:" << k;
            }
        }
    }
    ui->waterprintText->setDisabled(true);
}

void Widget::setConfig(QString key, QString value)
{
    QJsonObject jsonObj;
    jsonObj[key] = QString("%1").arg(value);
    QJsonDocument doc(jsonObj);
    QString a = QString::fromUtf8(doc.toJson(QJsonDocument::Compact).constData());
    m_dbusInterface->SetRecordItemValue(a);
    KLOG_DEBUG() << __func__ << "key: " << key << "value: " << value;
}

void Widget::sendSwitchControl(int from_pid, int to_pid, QString op)
{
    m_dbusInterface->SwitchControl(from_pid, to_pid, op);
    KLOG_DEBUG() << __func__ << "from: " << from_pid << "to: " << to_pid << "op: " << op;
}

QLabel *Widget::createVideoDurationLabel(QString duration)
{
    QLabel *durationLabel = new QLabel();
    durationLabel->setText(duration);
    durationLabel->setFont(QFont("Sans Serif", 10));
    durationLabel->setStyleSheet("color:#fff;");
    return durationLabel;
}

bool Widget::parseJsonData(const QString &param,  QJsonObject &jsonObj)
{
    QString str = param;
    QJsonParseError jError;
    QJsonDocument jsonDocument = QJsonDocument::fromJson(str.toUtf8(), &jError);

    if (jsonDocument.isObject())
    {
        jsonObj = jsonDocument.object();
        return true;
    }

    //判断是否因为中文utf8导致解析失败
    KLOG_INFO() << "parse json" << jsonDocument << "err, err info:" << jError.error;
    if (QJsonParseError::ParseError::IllegalUTF8String != jError.error)
        return false;

    //QJsonDocument::fromJson解析中文utf8失败处理
    if (!param.startsWith("{") || !param.endsWith("}"))
        return false;

    QStringList jsonList = param.split(",");
    for (auto jsonString : jsonList)
    {
        QStringList dataList = jsonString.split("\"");
        int index = dataList.indexOf(":");
        if (index > 0 && index < dataList.size() - 1)
        {
            jsonObj[dataList[index-1]] = dataList[index+1];
        }
    }

    return true;
}

void Widget::callNotifyProcess(QString op, int minutes)
{
    // 防止提示已录屏0分钟
    if (OPERATE_NOTIFY_TIMING == op && 0 == minutes)
        return;

    // 仅开始、暂停、重启、停止、定时、错误提醒需要提示
    if (OPERATE_RECORD_START == op || OPERATE_RECORD_PAUSE == op || OPERATE_RECORD_RESTART == op
        || OPERATE_RECORD_STOP == op || OPERATE_NOTIFY_TIMING == op  || OPERATE_NOTIFY_ERROR == op)
    {
        QProcess process;
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.insert("DISPLAY", getenv("DISPLAY")); // Add an environment variable
        env.insert("XAUTHORITY", getenv("XAUTHORITY"));
        process.setProcessEnvironment(env);
        QStringList arg;
        struct passwd *pw = getpwnam(getenv("USER"));
        uid_t uid = pw != nullptr ? pw->pw_uid : 0;
        arg << (QString("--record ") + op + QString(" ") + QString::number(uid) + QString(" ") + QString::number(minutes));
        process.start("/usr/bin/ks-vaudit-notify", arg);
        KLOG_INFO() << "notify info:" << op << "process"<< process.pid() << process.arguments();
        process.waitForFinished();
        process.close();
    }
}

void Widget::realClose()
{
    // 给后端发送exit信号
    sendSwitchControl(m_selfPID, m_recordPID, OPERATE_RECORD_EXIT);
    this->hide();
    m_pConfirm->hide();
    m_pConfirm->close();
    this->close();
}

void Widget::refreshTime(int from_pid, int to_pid, QString op)
{
    if (to_pid != m_selfPID)
        return;

    if (op.startsWith("totaltime")){
        if (from_pid != m_recordPID)
            return;

        QString timeText = op.split(" ")[1];
        ui->timeStamp->setText(timeText);
        m_heartBeat->start(HEARTBEAT_MS);
        // 屏幕定时提示处理
        if (m_timing != 0)
        {
            QStringList list = timeText.split(":");
            int minute = list[0].toInt() * 60 + list[1].toInt();
            if ((m_lastMinutes != minute) && ((minute % m_timing) == 0))
            {
                callNotifyProcess(OPERATE_NOTIFY_TIMING, minute);
                m_lastMinutes = minute;
            }
        }
    } else if (OPERATE_DISK_FRONT_NOTIFY == op){ //磁盘空间不足
        if (m_isRecording)
        {
            KLOG_INFO() << "receive DiskSpace from" << from_pid;
            // TODO: stop record op here
            Dialog *prompt = new Dialog(this, OPERATE_DISK_FRONT_NOTIFY);
            on_stopBtn_clicked();
            prompt->exec();
        }
    } else if (OPERATE_FRONT_PROCESS == op) { //接收后台进程pid
        m_recordPID = from_pid;
        KLOG_INFO() << "receive backend record pid:" << from_pid;
    } else if (op.endsWith(OPERATE_RECORD_DONE)) { //接收录屏进程操作结果
        KLOG_DEBUG() << "receive op done:" << op;
        QStringList list = op.split("-");
        if (list.size() == 2)
        {
            callNotifyProcess(list[0]);
        }
    } else {
        KLOG_DEBUG() << "from_pid:" << from_pid << "op:" << op;
    }
}

void Widget::receiveNotification(int pid, QString message)
{
    if (pid == m_recordPID && VAUDIT_ACTIVE == message)
    {
        // 在后台发送is_active信号后，再设置窗口位置，显示窗口，以防发送录屏信号后台没收到
        KLOG_INFO() << "receive backend record active info, and call show";
        this->move(m_toWidth, m_toHeight);
        this->show();
    }
}

void Widget::sendMicToConfig()
{
    // 滑块值变动，传递给配置中心
    int value = ui->audioSlider->value();
    setConfig(GENEARL_CONFIG_MIC_VOLUME, QString("%1").arg(value));
}

void Widget::sendSpkToConfig()
{
    // 滑块值变动，传递给配置中心
    int value = ui->volumnSlider->value();
    setConfig(GENEARL_CONFIG_SPEAKER_VOLUME, QString("%1").arg(value));
}

void Widget::reconnectMonitor()
{
    if (!m_isRecording){
        return;
    }
    // 点击停止按钮
    ui->stopBtn->click();
    // 重新拉起后台
    QString args = QString("process=%1;DISPLAY=%2;XAUTHORITY=%3;USER=%4;HOME=%5").arg(m_selfPID).arg(getenv("DISPLAY")).arg(getenv("XAUTHORITY")).arg(getenv("USER")).arg(getenv("HOME"));
    sendSwitchControl(m_selfPID, 0, args);
    callNotifyProcess(OPERATE_NOTIFY_ERROR);
}

void Widget::openActivate()
{
    m_activatePage->setFocus();
    m_activatePage->exec();
}

void Widget::setToCenter(int w, int h) {
    m_toWidth = w;
    m_toHeight = h;
}
