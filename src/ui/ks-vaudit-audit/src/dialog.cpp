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


void Dialog::initUI()
{
    if (m_dialogType == QString("exit")){
        ui->label->setText("关闭程序后将退出麒麟信安录屏软件，是否关闭程序？");
    }else if(m_dialogType == QString("noprivilege")){
        ui->label->setText("您无权限查看！");
    }
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

