#include "login.h"
#include "ui_login.h"
#include <QPushButton>
#include <QListView>
#include <QJsonDocument>
#include "kiran-log/qt5-log-i.h"
#include "ksvaudit-configure_global.h"

Login::Login(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Login)
{
    ui->setupUi(this);
    m_activation = new ActivatePage();

    if (!checkActivation()){
        m_activation->exec();
    }
    this->setWindowFlags(Qt::Dialog|Qt::FramelessWindowHint);
    this->setAttribute(Qt::WA_TranslucentBackground, true);
    m_dbusInterface = new ConfigureInterface(KSVAUDIT_CONFIGURE_SERVICE_NAME, KSVAUDIT_CONFIGURE_PATH_NAME, QDBusConnection::systemBus(), this);
    this->initUI();

}

Login::~Login()
{
    delete ui;
}

QJsonObject Login::getCurrentUserInfo()
{
    return m_currentUserInfo;
}

void Login::initUI()
{
    ui->label_3->hide();
    ui->userBox->setView(new QListView());
    getUserInfo();
}

void Login::on_accept_clicked()
{
    bool isActivated = checkActivation();
    if (!isActivated){
        m_activation->exec();
        checkActivation();
        return;
    }
    bool passwordConfirmed = checkLogin();
    if(passwordConfirmed){
        emit show_widget(m_currentUserInfo);
        this->hide();
        KLOG_INFO() << ui->userBox->currentText() << "login succeed!";
    }else{
        ui->label_3->show();
        ui->passwdEdit->setProperty("isError",true);
        ui->passwdEdit->style()->polish(ui->passwdEdit);
        KLOG_INFO() << ui->userBox->currentText() << "trying to login failed!";
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

void Login::getUserInfo()
{
    QString value = m_dbusInterface->GetUserInfo("{\"db_passwd\":\"12345678\"}");
    QJsonDocument doc = QJsonDocument::fromJson(value.toLatin1());
    if (doc.isArray()){
        m_userInfoArray = doc.array();
    }
}

bool Login::checkLogin()
{
//    KLOG_INFO() << "user selected:" << ui->userBox->currentText() << "passwd entered:" << ui->passwdEdit->text();
    QString currentText =  ui->userBox->currentText();
    QString currentPasswd = ui->passwdEdit->text();
    for (auto i : m_userInfoArray){
        QJsonObject o = i.toObject();
        KLOG_DEBUG() <<"origin:" << o["passwd"].toString() << "after:" << base64ToStr(o["passwd"].toString()) << "cur:" << QString::compare(o["passwd"].toString(), currentPasswd);
        if (o["user"] == currentText && base64ToStr(o["passwd"].toString()) == currentPasswd) {
            m_currentUserInfo = o;
            return true;
        }
    }
    return false;
}

QString Login::strTobase64(QString inputStr)
{
    QByteArray byteArr(inputStr.toStdString().data());
    return byteArr.toBase64();
}

QString Login::base64ToStr(QString inputStr)
{
    return QByteArray::fromBase64(QVariant(inputStr).toByteArray()).toStdString().data();
}

bool Login::checkActivation()
{
    // 检查是否激活并且设置ui
    m_isActivated = m_activation->getActivation();
    if (!m_isActivated){
        ui->dotWidget->setStyleSheet("border:none;"
                                     "border-radius:2px;"
                                     "background-color:#fa1919;");
        ui->activationBtn->setText("未激活");
    }else{
        ui->dotWidget->setStyleSheet("border:none;"
                                     "border-radius:2px;"
                                     "background-color:#2eb3ff;");
        ui->activationBtn->setText("已激活");
    }
    return m_isActivated;
}

void Login::on_exit_clicked()
{
    this->close();
}

void Login::show_logout()
{
    // 刷新用户信息
    this->checkActivation();
    this->getUserInfo();
    this->show();
}


void Login::on_passwdEdit_textChanged(const QString &arg1)
{
    ui->label_3->hide();
    ui->passwdEdit->setProperty("isError",false);
    ui->passwdEdit->style()->polish(ui->passwdEdit);
}


void Login::on_activationBtn_clicked()
{
    m_activation->exec();
    checkActivation();
}
