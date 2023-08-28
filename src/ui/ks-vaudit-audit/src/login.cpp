#include "login.h"
#include "ui_login.h"
#include <QPushButton>
#include <QListView>

Login::Login(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Login)
{
    ui->setupUi(this);
    this->initUI();
    this->setWindowFlags(Qt::Dialog|Qt::FramelessWindowHint);
    this->setAttribute(Qt::WA_TranslucentBackground, true);

}

Login::~Login()
{
    delete ui;
}

void Login::initUI()
{
    ui->comboBox->setView(new QListView());
}

void Login::on_accept_clicked()
{
    bool passwordConfirmed = true;
    if(passwordConfirmed){
        emit show_widget();
        this->close();
    }
}

void Login::mousePressEvent(QMouseEvent *event)
{
    if (event->pos().y() < 40 && event->button() == Qt::LeftButton){
        m_bDrag = true;
        mouseStartPoint = event->globalPos();
        windowTopLeftPoint = this->frameGeometry().topLeft();
    }
}

void Login::mouseMoveEvent(QMouseEvent *event)
{
    if (m_bDrag){
        QPoint  distance = event->globalPos() - mouseStartPoint;
        this->move(windowTopLeftPoint + distance);
    }
}

void Login::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton){
        m_bDrag = false;
    }
}

void Login::on_exit_clicked()
{
    this->close();
}
