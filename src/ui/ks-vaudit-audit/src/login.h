#ifndef LOGIN_H
#define LOGIN_H

#include <QDialog>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPoint>
#include "configure_interface.h"

namespace Ui {
class Login;
}

class Login : public QDialog
{
    Q_OBJECT

public:
    explicit Login(QWidget *parent = 0);
    ~Login();
    QJsonObject getCurrentUserInfo();

protected:
    void initUI();
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void getUserInfo();
    bool checkLogin();
    QString strTobase64(QString);
    QString base64ToStr(QString);

private slots:
    void on_accept_clicked();
    void on_exit_clicked();
    void show_logout();

    void on_passwdEdit_textChanged(const QString &arg1);

signals:
    void show_widget(QJsonObject);

private:
    Ui::Login *ui;
    bool m_bDrag = false;
    QPoint mouseStartPoint;
    QPoint windowTopLeftPoint;
    ConfigureInterface *m_dbusInterface;
    QJsonArray m_userInfoArray;
    QJsonObject m_currentUserInfo;

};

#endif // LOGIN_H
