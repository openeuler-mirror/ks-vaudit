#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QMouseEvent>
#include <QLineEdit>

namespace Ui {
class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = 0, QString dialogType = QString(""), QString fileName = QString(""));
    ~Dialog();
    QString getNewName();
    QString getOldName();

protected:
    void initUI();
    void initAboutUI();
    void initRenameUI();
    void initActivateUI(bool);

private slots:

    void on_accept_clicked();

    void on_cancel_clicked();

    void exitDialog();

    void emitRename();

signals:
    void close_window();
    void delete_file();
    void rename_file();

private:
    Ui::Dialog *ui;
    bool m_bDrag = false;
    QPoint mouseStartPoint;
    QPoint windowTopLeftPoint;
    QString m_dialogType;
    QLineEdit *m_fileNameEditor = NULL;
    QString m_fileName;
    QString m_oldName;

};

#endif // DIALOG_H
