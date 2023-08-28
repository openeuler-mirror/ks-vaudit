#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QMouseEvent>
#include <QPushButton>
#include <QIntValidator>
#include <QTimer>
#include <QFileInfo>
#include <QStandardItemModel>
#include <QAction>
#include <QLineEdit>
#include <dialog.h>
#include <QProcess>
#include <QJsonObject>
#include <QJsonDocument>
#include "configure_interface.h"


namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();

protected:
    void init_ui();
    void createList();
    void comboboxStyle();
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    QPushButton* createOperationBtn(int);
    QList<QFileInfo>* getVideos(QString path, QString regName);
    QString getVideoDuration(QString absPath);
    void refreshList(QString regName = QString(""));
    QLineEdit *createVideoNameEdit(QString fileName);
    void readConfig();
    void setConfigtoUi();
//    void setConfig(QString key, QString value);
    int startRecrodProcess();
    void sendSwitchControl(int from_pid, int to_pid, QString op);
    void testConfig(QString key, QString value);


private slots:
    void on_exit_clicked();

    void on_UserBtn_clicked();

    void on_ListBtn_clicked();

    void on_pushButton_clicked();

    void onTableBtnClicked();

    void on_ConfigBtn_clicked();

    void on_minimize_clicked();

    void on_fpsEdit_textChanged(const QString &arg1);

    void on_pushButton_2_clicked();

    void on_pushButton_3_clicked();

    void on_fpsEdit_returnPressed();

    void on_searchBar_returnPressed();

    void playVideo();

    void openDir();

    void on_searchBar_editingFinished();

    void on_clarityBox_currentIndexChanged(int index);

    void on_pauseBox_currentIndexChanged(int index);

    void on_typeBox_currentIndexChanged(int index);

    void realClose();

    void on_freeSpaceBox_currentIndexChanged(int index);

    void on_keepTimeBox_currentIndexChanged(int index);

    void on_maxRecordBox_currentIndexChanged(int index);

    void setConfig();

    void resetConfig();

    void on_editPasswordBtn_clicked();

    void on_cancelPasswordBtn_clicked();

    void show_widget_page();
    void on_AboutBtn_clicked();

signals:


private:
    Ui::Widget *ui;
    bool m_bDrag = false;
    QPoint mouseStartPoint;
    QPoint windowTopLeftPoint;
    bool m_isRecording = false;
    QString m_fps = "10";
    QIntValidator *m_intValidator;
    QStandardItemModel *m_model = NULL;
    QMenu *m_rightMenu = NULL;

    QAction *m_playAction = NULL;
//    QAction *m_renameAction = NULL;
    QAction *m_folderAction = NULL;
//    QAction *m_deleteAction = NULL;

    QList<QFileInfo> *m_fileList = NULL;
    QString m_regName = "";
    QLineEdit *m_videoNameEditor = NULL;

    Dialog *m_renameDialog = NULL;
    ConfigureInterface *m_dbusInterface;
    QString m_waterText;
    int m_recordPID = 0;
    int m_selfPID = 0;
    bool m_needRestart = false;
    QProcess *m_recordP;
    QJsonObject m_configure;
    QJsonObject m_toSetConf;
};

#endif // WIDGET_H
