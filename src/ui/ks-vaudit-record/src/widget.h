#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QMouseEvent>
#include <QPushButton>
#include <QTimer>
#include <QFileInfo>
#include <QStandardItemModel>
#include <QAction>
#include <QLineEdit>
#include <QProcess>
#include <QTimer>
#include <QStringList>

#include "dialog.h"
#include "configure_interface.h"
#include "kiran-log/qt5-log-i.h"
#include "activate-page.h"


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
    QPushButton* createOperationBtn(int modelIndex, int listIndex);
    QList<QFileInfo> getVideos(QString path, QString regName);
    QString getVideoDuration(QString absPath);
    void refreshList(QString regName = QString(""));
    QLineEdit *createVideoNameEdit(QString fileName);
    void readConfig();
    void setConfig(QString key, QString value);
    void sendSwitchControl(int from_pid, int to_pid, QString op);
    QLabel *createVideoDurationLabel(QString);
    bool parseJsonData(const QString &param,  QJsonObject &jsonObj);
    void callNotifyProcess(QString op, int minutes = 0);

private slots:
    void on_exit_clicked();
    void on_NormalBtn_clicked();
    void on_ListBtn_clicked();
    void on_pushButton_clicked();
    void onTableBtnClicked();
    void on_waterprintCheck_stateChanged(int arg1);
    void on_volumnBtn_clicked();
    void on_volumnSlider_valueChanged(int value);
    void on_audioBtn_clicked();
    void on_audioSlider_valueChanged(int value);
    void on_playBtn_clicked();
    void on_ConfigBtn_clicked();
    void on_minimize_clicked();
    void on_waterprintText_returnPressed();
    void on_searchBar_returnPressed();
    void on_waterprintConfirm_clicked();
    void on_waterprintText_textChanged(const QString &arg1);
    void on_resolutionBox_currentIndexChanged(int index);
    void on_audioBox_currentIndexChanged(int index);
    void on_clarityBox_currentIndexChanged(int index);
    void on_remainderBox_currentIndexChanged(int index);
    void on_typeBox_currentIndexChanged(int index);
    void playVideo();
    void openDir();
    void deleteVideo();
    void realDelete();
    void openAbout();
    void renameVideo();
    void realRename();
    void on_stopBtn_clicked();
    void realClose();
    void refreshTime(int, int, QString);
    void openActivate();
    void on_fpsBox_currentIndexChanged(int index);
    void receiveNotification(int, QString);

public:
    void setToCenter(int w, int h);

signals:


private:
    Ui::Widget *ui;
    bool m_bDrag = false;
    QPoint mouseStartPoint;
    QPoint windowTopLeftPoint;
    bool m_isRecording = false;
    QStandardItemModel *m_model = NULL;
    QMenu *m_rightMenu = NULL;
    QStringList m_fpsList;

    QAction *m_playAction = NULL;
    QAction *m_renameAction = NULL;
    QAction *m_folderAction = NULL;
    QAction *m_deleteAction = NULL;

    QList<QFileInfo> m_fileList;
    QString m_regName = "";
    QLineEdit *m_videoNameEditor = NULL;

    Dialog *m_renameDialog = NULL;
    ConfigureInterface *m_dbusInterface;
    QString m_waterText;
    int m_recordPID = 0;
    int m_selfPID = 0;
    bool m_needRestart = false;
    ActivatePage *m_activatePage;
    bool m_isActivated = false;
    Dialog *m_pConfirm;
    int m_timing{};
    int m_lastMinutes{};
    int m_toWidth;
    int m_toHeight;
};

#endif // WIDGET_H
