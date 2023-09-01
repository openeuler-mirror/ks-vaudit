#include "dialog.h"
#include "ui_dialog.h"
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

Dialog::Dialog(QWidget *parent, QString dialogType, QString fileName) :
    QDialog(parent),
    m_dialogType(dialogType),
    m_fileName(fileName),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::Dialog|Qt::FramelessWindowHint);
    this->setAttribute(Qt::WA_TranslucentBackground, true);
    this->setAttribute(Qt::WA_DeleteOnClose, true);
    initUI();
}

Dialog::~Dialog()
{
    delete ui;
}

QString Dialog::getOldName()
{
    return m_fileName;
}

QString Dialog::getNewName()
{
    QString leftFileName = m_fileNameEditor->text();
    QString rightFileName = m_fileName.right(4);
    return QString("%1%2").arg(leftFileName).arg(rightFileName);
}

QLabel *Dialog::getQrLabel()
{
    return m_qrCodeLabel;
}

void Dialog::initUI()
{
    if (m_dialogType == QString("exit")){
        ui->label->setText(tr("Confirm to exit Vaudit Record?"));
    }else if(m_dialogType == QString("delete")){
        ui->label->setText(tr("Confirm to delete the file?\nThe file cannot restore, please be cautious!"));
    }else if(m_dialogType == QString("about")){
        initAboutUI();
    }else if(m_dialogType == QString("rename")){
        initRenameUI();
    }else if(m_dialogType == QString("activate")){
        initActivateUI(true);
    }else if(m_dialogType == QString("activateFailed")){
        initActivateUI(false);
    }else if(m_dialogType == QString("qrcode")){
        initQRcode();
    }else if(m_dialogType == QString("DiskSpace")){
        ui->label->setText(tr("No free space on disk. Recording file saved. Please free disk storage and restart"));
    }else if(m_dialogType == QString("Warning")){
        ui->label->setText(tr("Rename failed, the file existed, please enter again!"));
    }
}

void Dialog::initAboutUI()
{
    // 如果是关于窗口，单独函数处理
    ui->titleText->setText(tr("About Software"));
    ui->label->hide();
    this->setFixedSize(684,338);
    ui->dialogWidget->setFixedSize(684,338);
    ui->titleBar->setFixedSize(682,40);
    QPushButton *exitButton = new QPushButton(ui->titleBar);
    exitButton->setGeometry(646,8,24,24);
    exitButton->setStyleSheet("background-color:transparent;"
                              "border:none;");
    exitButton->setIcon(QIcon(":/images/clear_icon.svg"));
    exitButton->setIconSize(QSize(13,13));
    connect(exitButton, SIGNAL(clicked()), this, SLOT(exitDialog()));
    ui->bodyWidget->hide();
    QWidget *aboutBody = new QWidget(ui->dialogWidget);
    aboutBody->setGeometry(4,44,676,290);
    aboutBody->setFixedSize(676,290);
    aboutBody->setStyleSheet("background-color:#2d2d2d;"
                             "border-radius:6px;");
    // ===============================================================
    QVBoxLayout *vLayout = new QVBoxLayout(aboutBody);
//    vLayout->setSizeConstraint(QVBoxLayout::SetMaximumSize);
//    aboutBody->setContentsMargins(24,0,24,0);
    vLayout->setContentsMargins(24,0,24,0);
    QWidget *logoWidget = new QWidget(aboutBody);
    logoWidget->setFixedSize(628,128);
    logoWidget->setStyleSheet(""
//                              "background-color:red;"
                              "image:url(:/images/128px.png);");
    // ---------------------------------------------------------------
    QWidget *vWidget1 = new QWidget(this);
    vWidget1->setFixedSize(628,36);
    vWidget1->setStyleSheet("background-color:#393939;"
                            "color:#fff;");
    QHBoxLayout *hwLayou1 = new QHBoxLayout(vWidget1);

    QLabel *label1 = new QLabel(vWidget1);
    label1->setText(tr("Software version"));
    label1->setFont(QFont("Sans Serif", 12));
    QLabel *label2 = new QLabel(vWidget1);
    label2->setText("v1.0");
    label2->setFont(QFont("Sans Serif", 12));
    label2->setStyleSheet("color:#919191;");
    hwLayou1->addWidget(label1);
    hwLayou1->addStretch();
    hwLayou1->addWidget(label2);
    // ---------------------------------------------------------------
    QWidget *vWidget2 = new QWidget(this);
    vWidget2->setFixedSize(628,36);
    vWidget2->setStyleSheet("background-color:#393939;"
                            "color:#fff;");
    QHBoxLayout *hwLayou2 = new QHBoxLayout(vWidget2);

    QLabel *label3 = new QLabel(vWidget2);
    label3->setText(tr("Copyright infomation"));
    label3->setFont(QFont("Sans Serif", 12));
    QLabel *label4 = new QLabel(vWidget2);
    label4->setText("Copyright ©2022-2023 KylinSec Co.Ltd.All Rights Reserved.");
    label4->setFont(QFont("Sans Serif", 12));
    label4->setStyleSheet("color:#919191;");
    hwLayou2->addWidget(label3);
    hwLayou2->addStretch();
    hwLayou2->addWidget(label4);
    // ---------------------------------------------------------------
    vLayout->addSpacing(36);
    vLayout->addWidget(logoWidget);
    vLayout->addStretch();
    vLayout->addWidget(vWidget1);
    vLayout->addSpacing(5);
    vLayout->addWidget(vWidget2);
    vLayout->addSpacing(16);
}

void Dialog::initRenameUI()
{
    ui->titleText->setText(tr("Rename"));
    ui->label->hide();
//    m_oldName = m_fileName;
//    ui->bodyWidget->setFocusPolicy(Qt::ClickFocus);
    m_fileNameEditor = new QLineEdit(ui->bodyWidget);
    QString fileBaseName = m_fileName.left(m_fileName.lastIndexOf("."));
    m_fileNameEditor->setText(fileBaseName);
    m_fileNameEditor->setStyleSheet("QLineEdit{"
                                    "color:#fff;"
                                    "background-color:#222222;"
                                    "border-radius:6px;"
                                    "border:1px solid #393939;"
                                    "padding-left:10px;"
                                    "padding-right:10px;"
                                    "}"
                                    "QLineEdit:hover{"
                                    "color:#fff;"
                                    "background-color:#222222;"
                                    "border-radius:6px;"
                                    "border:1px solid #2eb3ff;"
                                    "}");
    m_fileNameEditor->setGeometry(24,24,264,36);
//	 connect(m_fileNameEditor, SIGNAL(returnPressed()), this, SLOT());
    connect(m_fileNameEditor, SIGNAL(textEdited(QString)), this, SLOT(checkName(QString)));
    m_warningLabel = new QLabel(ui->bodyWidget);
    m_warningLabel->setGeometry(24,53,264,36);
    m_warningLabel->setAlignment(Qt::AlignCenter);
    m_warningLabel->setStyleSheet("color:#fa1919;");
    m_warningLabel->setText(tr("Rename failed, please re-enter"));
    m_warningLabel->hide();
    m_fileNameEditor->setFocus();
    m_fileNameEditor->selectAll();
}

void Dialog::initActivateUI(bool isSucceed)
{
    ui->titleText->setText(tr("Software Activate"));
    if (isSucceed){
        ui->label->setText(tr("Software activated success!"));
    }else{
        ui->label->setText(tr("Activate failed, please correct activation code!"));
    }
}

void Dialog::initQRcode()
{
    ui->titleText->setText(tr("Machine code"));
    this->setFixedSize(320,276);
    ui->dialogWidget->setFixedSize(320,276);
    ui->label->hide();
    ui->accept->hide();
    ui->cancel->hide();

    QPushButton *exitButton = new QPushButton(ui->titleBar);
    exitButton->setGeometry(278,0,40,40);
    exitButton->setStyleSheet("background-color:transparent;"
                              "border:none;");
    exitButton->setIcon(QIcon(":/images/clear_icon.svg"));
    exitButton->setIconSize(QSize(12,12));
    connect(exitButton, SIGNAL(clicked()), this, SLOT(exitDialog()));
    ui->bodyWidget->setFixedSize(312,227);

    QVBoxLayout *vlayout = new QVBoxLayout(ui->bodyWidget);
    vlayout->setContentsMargins(66,0,66,0);
    m_qrCodeLabel = new QLabel();
    m_qrCodeLabel->setFixedSize(180,180);
    m_qrCodeLabel->setStyleSheet("background-color:green;");

    QLabel *label_1 = new QLabel();
    label_1->setStyleSheet("color:#fff;font-size:13px;");
    label_1->setText(tr("Scan to obtain the machine code"));
    label_1->setAlignment(Qt::AlignCenter);
    vlayout->addSpacing(10);
    vlayout->addWidget(m_qrCodeLabel);
    vlayout->addWidget(label_1);
    vlayout->addStretch();
}

void Dialog::on_accept_clicked()
{
    if (m_dialogType == QString("exit")){
        emit close_window();
    }else if(m_dialogType == QString("delete")){
        emit delete_file();
    }else if(m_dialogType == QString("rename")){
        emit rename_file();
    }
    this->close();
}

void Dialog::on_cancel_clicked()
{
    this->close();
}

void Dialog::exitDialog()
{
    this->close();
}

void Dialog::emitRename()
{
    emit rename_file();
}

void Dialog::checkName(QString inputName)
{
    if (inputName.isEmpty() || inputName.startsWith(".") || inputName.startsWith("/")){
        ui->accept->setDisabled(true);
        m_warningLabel->show();
    }else{
        ui->accept->setDisabled(false);
        m_warningLabel->hide();
    }
}
