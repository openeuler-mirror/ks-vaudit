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
    return m_fileNameEditor->text();
}

void Dialog::initUI()
{
    if (m_dialogType == QString("exit")){
        ui->label->setText("关闭程序后将退出麒麟信安录屏软件，是否关闭程序？");
    }else if(m_dialogType == QString("delete")){
        ui->label->setText("是否确认删除录屏文件？\n删除后将无法恢复，请谨慎操作！");
    }else if(m_dialogType == QString("about")){
        initAboutUI();
    }else if(m_dialogType == QString("rename")){
        initRenameUI();
    }
}

void Dialog::initAboutUI()
{
    // 如果是关于窗口，单独函数处理
    ui->titleText->setText("关于软件");
    ui->label->hide();
    this->setFixedSize(684,338);
    ui->dialogWidget->setFixedSize(684,338);
    ui->titleBar->setFixedSize(682,40);
    QPushButton *exitButton = new QPushButton(ui->titleBar);
    exitButton->setGeometry(646,8,24,24);
    exitButton->setStyleSheet("background-color:transparent;"
                              "border:none;");
    exitButton->setIcon(QIcon(":/images/clear_icon.svg"));
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
    label1->setText("软件版本号");
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
    label3->setText("版权信息");
    label3->setFont(QFont("Sans Serif", 12));
    QLabel *label4 = new QLabel(vWidget2);
    label4->setText("© 2006-2020 版权归湖南麒麟信安最终所有");
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
    ui->titleText->setText("重命名");
    ui->label->hide();
//    m_oldName = m_fileName;
//    ui->bodyWidget->setFocusPolicy(Qt::ClickFocus);
    m_fileNameEditor = new QLineEdit(ui->bodyWidget);
    m_fileNameEditor->setText(m_fileName);
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
    connect(m_fileNameEditor, SIGNAL(returnPressed()), this, SLOT(emitRename()));
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
