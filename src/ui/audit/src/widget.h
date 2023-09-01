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
#include <dialog.h>
#include <QProcess>
#include <QJsonObject>
#include <QJsonDocument>
#include <QStringList>
#include "configure_interface.h"
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
    void initAudadmUi();
    void initSysadmUi();
    void initSecadmUi();
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
    void setConfigtoUi();
    int startRecrodProcess();
    void sendSwitchControl(int from_pid, int to_pid, QString op);
    void testConfig(QString key, QString value);
    QString checkNewPassword(QString);
    QString strTobase64(QString inputStr);
    QString base64ToStr(QString inputStr);
    void getActivationInfo();
    QLabel *createVideoDurationLabel(QString duration);

private slots:
    void on_exit_clicked();
    void on_UserBtn_clicked();
    void on_ListBtn_clicked();
    void on_pushButton_clicked();
    void onTableBtnClicked();
    void on_ConfigBtn_clicked();
    void on_minimize_clicked();
    void on_searchBar_returnPressed();
    void playVideo();
    void openDir();
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
    void show_widget_page(QJsonObject);
    void on_AboutBtn_clicked();
    void on_logoutBtn_clicked();
    void on_savePasswordBtn_clicked();
    void on_oldPasswordEdit_textChanged(const QString &arg1);
    void on_confirmPasswordEdit_textChanged(const QString &arg1);
    void on_newPasswordEdit_textChanged(const QString &arg1);
    void on_aboutInfoBtn_clicked();
    void on_fpsBox_currentIndexChanged(int index);

signals:
    void log_out();

private:
    Ui::Widget *ui;
    bool m_bDrag = false;
    QPoint mouseStartPoint;
    QPoint windowTopLeftPoint;
    QStandardItemModel *m_model = NULL;
    QMenu *m_rightMenu = NULL;
    QStringList m_fpsList;

    QAction *m_playAction = NULL;
    QAction *m_folderAction = NULL;

    QList<QFileInfo> m_fileList;
    QString m_regName = "";

    ConfigureInterface *m_dbusInterface;
    QJsonObject m_configure;
    QJsonObject m_toSetConf;
    QJsonObject m_currentUserInfo;
    ActivatePage *m_activatedPage;
    bool m_isActivated = false;
    QString m_expireDate;
};

#endif // WIDGET_H
