#include "dialog.h"
#include "ui_dialog.h"
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

Dialog::Dialog(QWidget *parent, QString dialogType) :
    QDialog(parent),
    m_dialogType(dialogType),
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

QLabel *Dialog::getQrLabel()
{
    return m_qrCodeLabel;
}

void Dialog::initUI()
{
    if (m_dialogType == QString("exit")){
        ui->label->setText("关闭程序后将退出麒麟信安录屏软件，是否关闭程序？");
    }else if(m_dialogType == QString("noprivilege")){
        ui->label->setText("您无权限查看！");
    }else if(m_dialogType == QString("activate")){
        initActivateUI(true);
    }else if(m_dialogType == QString("activateFailed")){
        initActivateUI(false);
    }else if(m_dialogType == QString("qrcode")){
        initQRcode();
    }
}

void Dialog::initActivateUI(bool isSucceed)
{
    ui->titleText->setText("软件激活");
    if (isSucceed){
        ui->label->setText("软件激活成功！");
    }else{
        ui->label->setText("激活失败，请检查激活码是否正确！");
    }
}

void Dialog::initQRcode()
{
    ui->titleText->setText("机器码");
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
    label_1->setText("扫一扫获取机器码");
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

