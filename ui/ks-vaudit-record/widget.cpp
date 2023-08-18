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
#include <QProcess>
#include <QDesktopServices>
#include <QUrl>
#include <QHBoxLayout>
#include <QDBusMessage>
#include <QDBusConnection>
#include <QJsonDocument>
#include <QJsonObject>
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
//    setAutoFillBackground(false);
    setWindowFlags(Qt::FramelessWindowHint);
//    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);

    m_dbusInterface = new  ComKylinsecKiranVauditConfigureInterface(KSVAUDIT_CONFIGURE_SERVICE_NAME, KSVAUDIT_CONFIGURE_PATH_NAME, QDBusConnection::systemBus(), this);
    //connect(sf, SIGNAL(ConfigureChanged(QString, QString)), this, SLOT(updateData(QString, QString)));
//    connect(m_dbusInterface, SIGNAL(ConfigureChanged(QString)), this, SLOT(updateData(QString)));
//    connect(m_dbusInterface, SIGNAL(SignalSwitchControl(int, QString)), this, SLOT(switchControl(int, QString)));

    init_ui();

}

Widget::~Widget()
{
    delete ui;
//    if (m_flashTimer){
//        delete m_flashTimer;
//        m_flashTimer = nullptr;
//    }
}


void Widget::init_ui()
{
    qDebug("here");
    ui->NormalBody->show();
    ui->NormalFooterBar->show();
    ui->ListBody->hide();
    ui->listArrow->hide();
    ui->configArrow->hide();
    ui->ConfigBody->hide();
    refreshList(m_regName);
    comboboxStyle();
    m_fps = ui->fpsEdit->text();
    m_intValidator = new QIntValidator(2, 60, this);
    ui->fpsEdit->setValidator(m_intValidator);
    ui->waterprintText->setMaxLength(20);
    QMenu* moreMenu = new QMenu();
    moreMenu->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
    moreMenu->setAttribute(Qt::WA_TranslucentBackground);
    moreMenu->setObjectName("moreMenu");
//    moreMenu->addAction(tr("软件激活"));
    QAction* aboutAction = moreMenu->addAction(tr("关于软件"));
    connect(aboutAction, SIGNAL(triggered()), this, SLOT(openAbout()));

    ui->moreMenuBtn->setMenu(moreMenu);

    // 列表右键弹出菜单
    m_rightMenu = new QMenu(this);
    m_rightMenu->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
    m_rightMenu->setAttribute(Qt::WA_TranslucentBackground);
    m_rightMenu->setObjectName("listMenu");
    m_playAction = m_rightMenu->addAction(tr("播放"));
    m_renameAction = m_rightMenu->addAction(tr("重命名"));
    m_folderAction = m_rightMenu->addAction(tr("文件位置"));
    m_deleteAction = m_rightMenu->addAction(tr("删除"));

    connect(m_playAction, SIGNAL(triggered()), this, SLOT(playVideo()));
    connect(m_renameAction, SIGNAL(triggered()), this, SLOT(renameVideo()));
    connect(m_folderAction, SIGNAL(triggered()), this, SLOT(openDir()));
    connect(m_deleteAction, SIGNAL(triggered()), this, SLOT(deleteVideo()));

    readConfig();
}

void Widget::comboboxStyle(){
    ui->clarityBox->setView(new QListView());
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
    Dialog *confirm = new Dialog(this, QString("exit"));
    connect(confirm, SIGNAL(close_window()), this, SLOT(close()));
    confirm->exec();
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
    refreshList();

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
        if (dirPath.startsWith(homePath)){
            dirPath.replace(0,homePath.size(),"~");
        }
        ui->pathLabel->setText(dirPath);
        ui->pathLabel->setToolTip(dirPath);
        refreshList(m_regName);
    }else{
        qDebug("Invalid path, abort!");
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
    m_deleteAction->setProperty("S_FILEPATH", testList->at(a).filePath());
    m_renameAction->setProperty("S_OLDNAME", testList->at(a).fileName());
    m_renameAction->setProperty("S_OLDPATH", testList->at(a).filePath());

    m_rightMenu->popup(QCursor::pos());
}

void Widget::on_waterprintCheck_stateChanged(int arg1)
{
    if (arg1 == 0){
        qDebug("hide waterprint");
        ui->waterprintText->hide();
        ui->waterprintConfirm->hide();
        ui->label_6->hide();
    }else if (arg1 == 2){
        qDebug("show waterprint");
        ui->waterprintText->show();
        ui->waterprintConfirm->show();
        ui->label_6->show();
    }
}

void Widget::on_volumnBtn_clicked()
{
    if(ui->volumnSlider->value() > 0){
        ui->volumnSlider->setValue(0);
        ui->volumnBtn->setStyleSheet("image:url(:/images/v0.svg);border:none;");
    }else if (ui->volumnSlider->value() == 0){
        ui->volumnSlider->setValue(100);
    }
}

void Widget::on_volumnSlider_valueChanged(int value)
{
    if (value == 0){
        ui->volumnBtn->setStyleSheet("image:url(:/images/v3.svg);border:none;");
    }else if (value < 50){
        ui->volumnBtn->setStyleSheet("image:url(:/images/v2.svg);border:none;");
    }else if (value >=50){
        ui->volumnBtn->setStyleSheet("image:url(:/images/v1.svg);border:none;");
    }
}

void Widget::on_audioBtn_clicked()
{
    if(ui->audioSlider->value() > 0){
        ui->audioSlider->setValue(0);
    }else if (ui->audioSlider->value() == 0){
        ui->audioSlider->setValue(100);
    }
}

void Widget::on_audioSlider_valueChanged(int value)
{
    if (value == 0){
        ui->audioBtn->setStyleSheet("image:url(:/images/m0.svg);border:none;");
    }else if (value > 0){
        ui->audioBtn->setStyleSheet("image:url(:/images/m1.svg);border:none;");
    }
}

void Widget::on_playBtn_clicked()
{
    if (ui->fpsEdit->text() == "1"){
        ui->fpsEdit->setText("10");
    }
    m_isRecording = !m_isRecording;
    if (m_isRecording){
        Widget::showMinimized();
        ui->playBtn->setStyleSheet("image:url(:/images/pauseRecord.svg);border:none;");
        qDebug("Start record screen!");
        ui->waterprintConfirm->setDisabled(true);
        ui->waterprintCheck->setDisabled(true);
    }else{
        ui->playBtn->setStyleSheet("image:url(:/images/record.svg);border:none;");
        ui->waterprintConfirm->setDisabled(false);
        ui->waterprintCheck->setDisabled(false);
        qDebug("Pause record screen!");
    }
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

void Widget::on_fpsEdit_textChanged(const QString &arg1)
{
    QString a = arg1;
    if (a == "1"){
        // 1开头的时候边框置红，不能开始录屏
        ui->fpsEdit->setStyleSheet("background-color:#222222;"
                                   "border:1px solid #ff4444;"
                                   "border-radius:6px;"
                                   "color:#fff;"
                                   "padding-left:10px;");
        ui->label_11->setStyleSheet("color:#ff4444");
//        m_hasError = ui->ConfigBtn;
    }else if (a.startsWith("0") || a.length() == 0){
        // 有种情况是0000000开头，强行设为最低2
        ui->fpsEdit->setText("2");
        ui->fpsEdit->setStyleSheet("background-color:#222222;"
                                   "border:1px solid #393939;"
                                   "border-radius:6px;"
                                   "color:#fff;"
                                   "padding-left:10px;");
        ui->label_11->setStyleSheet("color:#999999");
//        m_hasError = NULL;
    }else{
        ui->fpsEdit->setStyleSheet("background-color:#222222;"
                                   "border:1px solid #393939;"
                                   "border-radius:6px;"
                                   "color:#fff;"
                                   "padding-left:10px;");
        ui->label_11->setStyleSheet("color:#999999");
//        m_hasError = NULL;
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

void Widget::on_waterprintText_returnPressed()
{
    if (ui->waterprintText->hasFocus()){
        ui->waterprintText->clearFocus();
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


void Widget::openAbout()
{
    Dialog *aboutDialog = new Dialog(this,QString("about"));
    aboutDialog->exec();
}

void Widget::renameVideo()
{
    QString oldName = sender()->property("S_OLDNAME").toString();
//    qDebug() << oldName;
    if (m_renameDialog != NULL){
        delete m_renameDialog;
        m_renameDialog = NULL;
    }
    m_renameDialog = new Dialog(this, QString("rename"), oldName);
    connect(m_renameDialog, SIGNAL(rename_file()), this, SLOT(realRename()));
    m_renameDialog->exec();
}

void Widget::realRename()
{
    QString oldName = m_renameDialog->getOldName();
    QString newName = m_renameDialog->getNewName();
    for (int i = 0; i < m_fileList->size(); ++i){
        qDebug() << m_fileList->at(i).fileName() << ":" << oldName;
        if (m_fileList->at(i).fileName() == oldName){
            QFile file(m_fileList->at(i).filePath());
            qDebug() << "is ok: " << file.rename(QString("%1/%2").arg(m_fileList->at(i).absolutePath()).arg(newName));
            refreshList();
        }
    }
}

void Widget::playVideo()
{
    QString filePath = sender()->property("S_FILEPATH").toString();
    QProcess *p = new QProcess(this);
    p->setProcessChannelMode(QProcess::MergedChannels);
    QStringList commands;
//    commands << "-slave";
    commands << "-quiet";
    commands << filePath;
    // 采用分离式启动(startDetached)mplayer，因为和本录屏软件无关
    p->startDetached("/usr/bin/mplayer", commands);
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
//        qDebug() << "isFile: " << fileInfo.isFile();
        QFile::remove(filePath);
        refreshList(m_regName);
    }else{
        qDebug() << "No such file";
    }
}

QPushButton *Widget::createOperationBtn(int index)
{
    QPushButton *aBtn = new QPushButton();
    // 设置图标样式
    aBtn->setStyleSheet("QPushButton{"
                        "border:none;"
                        "background-color:transparent;"
                        "image:url(:/images/操作icon.svg);"
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
    if (avformat_open_input(&pCtx, absPath.toStdString().c_str(), NULL, NULL) < 0){
        qDebug() << "No such file!";
    }else{
        if (pCtx->duration != AV_NOPTS_VALUE){
            int64_t duration = pCtx->duration + (pCtx->duration <= INT64_MAX - 5000 ? 5000 : 0);
            secs = duration / AV_TIME_BASE;
            mins = secs / 60;
            secs %= 60;
            hours = mins / 60;
            mins %= 60;
//            qDebug() << "hh:mm:ss: " << hours << ":" << mins << ":" << secs;
        }else{
            qDebug() << "No duration";
        }
    }
    if (pCtx != NULL){
        avformat_close_input(&pCtx);
    }
    // 目前假设单个视频文件不超过99小时
    return QString("%1:%2:%3").arg(hours,2,10,QLatin1Char('0')).arg(mins,2,10,QLatin1Char('0')).arg(secs,2,10,QLatin1Char('0'));
}

void Widget::createList(){
    // 构建模型表头
    m_model = new QStandardItemModel();
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
    ui->videoList->setColumnWidth(1,100);
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
    if (m_model != NULL){
        m_model->clear();
        m_model = NULL;
    }
    createList();
    // 添加内容
    QList<QFileInfo>* testList;
    testList = getVideos(ui->pathLabel->text(), regName);
    m_fileList = testList;

    for (int p = 0; p < testList->size(); ++p){
        QString fileName = testList->at(p).fileName();
        QString duration = getVideoDuration(testList->at(p).absoluteFilePath());
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

        m_model->setItem(p, 0, new QStandardItem());
        ui->videoList->setIndexWidget(m_model->index(p,0), createVideoNameEdit(fileName));
        m_model->setItem(p, 1, new QStandardItem(duration));
        m_model->item(p,1)->setFont( QFont("Sans Serif", 10) );
        m_model->setItem(p, 2, new QStandardItem(fileSize));
        m_model->item(p,2)->setFont( QFont("Sans Serif", 10) );
        m_model->setItem(p, 3, new QStandardItem(modifyDate));
        m_model->item(p,3)->setFont( QFont("Sans Serif", 10) );
        m_model->setItem(p, 4, new QStandardItem());
        ui->videoList->setIndexWidget(m_model->index(p,4), createOperationBtn(p));
    }

}

QLineEdit *Widget::createVideoNameEdit(QString fileName)
{
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

QJsonDocument Widget::readConfig()
{
    QString value = m_dbusInterface->GetRecordInfo();
    QJsonDocument doc = QJsonDocument::fromJson(value.toLatin1());
    if (doc.isObject())
    {
        QJsonObject jsonObj = doc.object();
        for (auto k : jsonObj.keys())
        {
            qWarning() << __func__ << "GetRecordInfo" << k << jsonObj[k].toString();
            if (k == "FilePath"){
                ui->pathLabel->setText(jsonObj[k].toString());
            }else if(k == "FileType"){
                int setValue = 0;
                if (jsonObj[k].toString() == QString("ogv")){
                    setValue = 1;
                }
                ui->typeBox->setCurrentIndex(setValue);
            }else if(k == "Fps"){
                ui->fpsEdit->setText(jsonObj[k].toString());
            }else if(k == "Quality"){
                int value = 0;
                ui->clarityBox->setCurrentIndex(jsonObj[k].toInt(value));
            }else if(k == "RecordAudio"){
                int value = 0;
                ui->audioBox->setCurrentIndex(jsonObj[k].toInt(value));
            }else if(k == "RecordVideo"){
                int value = 0;
                ui->resolutionBox->setCurrentIndex(jsonObj[k].toInt(value));
            }else if(k == "TimingReminder"){
                int value = 0;
                ui->remainderBox->setCurrentIndex(jsonObj[k].toInt(value));
            }else if(k == "WaterPrint"){
                int value = 0;
                ui->waterprintCheck->setChecked(jsonObj[k].toInt(value));
            }else if(k == "WaterPrintText"){
                ui->waterprintText->setText(jsonObj[k].toString());
            }
        }
    }
    return doc;
//    printf("%s, %d\n", __func__, __LINE__);
}
