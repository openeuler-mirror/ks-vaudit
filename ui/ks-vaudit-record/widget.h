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
#include "vauditconfigure_interface.h"

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
    QJsonDocument readConfig();

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

    void on_fpsEdit_textChanged(const QString &arg1);

    void on_pushButton_2_clicked();

    void on_pushButton_3_clicked();

    void on_fpsEdit_returnPressed();

    void on_waterprintText_returnPressed();

    void on_searchBar_returnPressed();

    void playVideo();

    void openDir();

    void deleteVideo();

    void realDelete();

    void on_searchBar_editingFinished();

    void openAbout();

    void renameVideo();

    void realRename();


signals:


private:
    Ui::Widget *ui;
    bool m_bDrag = false;
    QPoint mouseStartPoint;
    QPoint windowTopLeftPoint;
    int m_isRecording = 0;
    QString m_fps = "10";
    QIntValidator *m_intValidator;
    QStandardItemModel *m_model = NULL;
    QMenu *m_rightMenu = NULL;

    QAction *m_playAction = NULL;
    QAction *m_renameAction = NULL;
    QAction *m_folderAction = NULL;
    QAction *m_deleteAction = NULL;

    QList<QFileInfo> *m_fileList = NULL;
    QString m_regName = "";
    QLineEdit *m_videoNameEditor = NULL;

    Dialog *m_renameDialog = NULL;
    ComKylinsecKiranVauditConfigureInterface *m_dbusInterface;
};

#endif // WIDGET_H
