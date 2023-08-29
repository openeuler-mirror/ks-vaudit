#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QMouseEvent>
#include <QLineEdit>
#include <QLabel>

namespace Ui {
class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = 0, QString dialogType = QString(""));
    ~Dialog();
    QLabel *getQrLabel();

protected:
    void initUI();
    void initAboutUI();
    void initRenameUI();
    void initActivateUI(bool);
    void initQRcode();

private slots:
    void on_accept_clicked();
    void on_cancel_clicked();
    void exitDialog();


signals:
    void close_window();

private:
    Ui::Dialog *ui;
    bool m_bDrag = false;
    QPoint mouseStartPoint;
    QPoint windowTopLeftPoint;
    QString m_dialogType;
    QLabel *m_qrCodeLabel;

};

#endif // DIALOG_H
