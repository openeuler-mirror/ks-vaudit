#ifndef ACTIVATE_PAGE_H
#define ACTIVATE_PAGE_H

#include <QDialog>
#include <QWidget>
#include <QPushButton>
#include <QLineEdit>

class ActivatePage : public QDialog
{
    Q_OBJECT
public:
    explicit ActivatePage(QWidget *parent = 0);
    ~ActivatePage();
    bool getActivation();
    QString getExpireDate();

protected:
    void initUI();
    void getLicenseInfo();

private slots:
    void acceptBtnClicked();

private:
    bool m_isActivated = false;
    QString m_machineCode, m_activateCode, m_expiredDate;
    QPushButton *m_activateBtn;
    QLineEdit *m_licenseCodeEdit;

};


#endif // ACTIVATE_PAGE_H
