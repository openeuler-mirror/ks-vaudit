#include "activate-page.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <qrencode.h>
#include <QPainter>
#include "dialog.h"
#include "license-entry.h"
#include "kiran-log/qt5-log-i.h"

ActivatePage::ActivatePage(QWidget *parent) : QDialog(parent)
{
    m_dateCodeEdit = new QLineEdit();
    this->getLicenseInfo();
    this->initUI();

    this->setWindowFlags(Qt::Dialog|Qt::FramelessWindowHint);
    this->setAttribute(Qt::WA_TranslucentBackground, true);

}

ActivatePage::~ActivatePage()
{
    if (m_licenseCodeEdit){
        delete m_licenseCodeEdit;
        m_licenseCodeEdit = NULL;
    }
    if (m_dateCodeEdit){
        delete m_dateCodeEdit;
        m_dateCodeEdit = NULL;
    }
}

bool ActivatePage::getActivation()
{
    // 获取最新的激活状态
    getLicenseInfo();
    return m_isActivated;
}

void ActivatePage::initUI()
{
    this->setFixedSize(640,364);
    this->setWindowIcon(QIcon(":/images/16px.png"));
    this->setWindowTitle("软件激活");
    // ---------------- Main Widget ------------------
    QWidget *ActivatedWidget = new QWidget(this);
    ActivatedWidget->setGeometry(0,0,640,364);
    ActivatedWidget->setObjectName("ActivatedWidget");
    ActivatedWidget->setStyleSheet("QWidget#ActivatedWidget{"
                                   "background:#222222;"
                                   "color:#fff;"
                                   "border:0.5px solid #464646;"
                                   "border-radius: 6px;"
                                   "}"
                                   "QLabel{"
                                   "color:#fff;"
                                   "font-size:14px;"
                                   "}"
                                   "QLineEdit{"
                                   "background-color:#222222;"
                                   "border:1px solid #393939;"
                                   "border-radius:6px;"
                                   "color:#fff;"
                                   "padding-left:10px;"
                                   "font-size:13px;"
                                   "}"
                                   "QLineEdit:hover{"
                                   "background-color:#222222;"
                                   "border:1px solid #2eb3ff;"
                                   "border-radius:6px;"
                                   "color:#fff;"
                                   "padding-left:10px;"
                                   "}"
                                   "QLineEdit:disabled{"
                                   "background-color:#393939;"
                                   "border-radius:6px;"
                                   "color:#919191;"
                                   "padding-left:10px;"
                                   "}");
    // ---------------- Title Bar --------------------
    QWidget *titleBar = new QWidget(ActivatedWidget);
    titleBar->setObjectName("titleBar");
    titleBar->setStyleSheet("QWidget#titleBar{"
                            "background-color:#2d2d2d;"
                            "border-top-left-radius:6px;"
                            "border-top-right-radius:6px;"
                            "}");
    titleBar->setGeometry(1,1,638,39);
    QPushButton *closeBtn = new QPushButton(titleBar);
    connect(closeBtn, SIGNAL(clicked()), this, SLOT(close()));
    closeBtn->setGeometry(600,2,36,36);
    closeBtn->setStyleSheet("QPushButton{"
                            "background-color:#2d2d2d;"
                            "border:none;"
                            "}");
    closeBtn->setIcon(QIcon(":/images/clear_icon.svg"));
    closeBtn->setIconSize(QSize(12,12));
    QWidget *logoWidget = new QWidget(titleBar);
    QLabel *titleLabel = new QLabel(titleBar);
    logoWidget->setGeometry(16,12,16,16);
    logoWidget->setStyleSheet("image:url(:/images/16px.png);border:none");
    titleLabel->setText("软件激活");
    titleLabel->setGeometry(42,4,100,30);
    titleLabel->setStyleSheet("color:#fff;font-size:12px;");

    // ---------------- Main Body --------------------
    QWidget *bodyWidget = new QWidget(ActivatedWidget);
    bodyWidget->setObjectName("bodyWidget");
    bodyWidget->setGeometry(4,44,632,316);
    bodyWidget->setStyleSheet("QWidget#bodyWidget{"
                              "background-color:#2d2d2d;"
                              "color:#fff;"
                              "border-radius:6px;"
                              "}");
    QVBoxLayout *mainVLayout = new QVBoxLayout(bodyWidget);
    mainVLayout->setContentsMargins(24,0,24,0);
    QLabel *licenseCodeLabel = new QLabel("激活码");
    m_licenseCodeEdit = new QLineEdit();
    m_licenseCodeEdit->setFixedSize(584,36);

    QLabel *machineCodeLabel = new QLabel("机器码");
    QLineEdit *machineCodeEdit = new QLineEdit();
    machineCodeEdit->setStyleSheet("QLineEdit{"
                                   "background-color:#393939;"
                                   "border-radius:6px;"
                                   "color:#919191;"
                                   "padding-left:10px;"
                                   "}"
                                   "QLineEdit:hover{"
                                   "border:1px solid #393939;"
                                   "}");
    machineCodeEdit->setFixedSize(584,36);
    QHBoxLayout *hlayout = new QHBoxLayout(machineCodeEdit);
    QPushButton *qrBtn = new QPushButton();
    connect(qrBtn,SIGNAL(clicked()),this,SLOT(showQR()));
    qrBtn->setFlat(true);
    qrBtn->setStyleSheet("QPushButton{"
                         "border-image: url(:/images/QC.svg);"
                         "}");
    qrBtn->setFixedSize(16,16);
    machineCodeEdit->setTextMargins(0,0,qrBtn->width(),0);
    machineCodeEdit->setReadOnly(true);
    hlayout->setContentsMargins(6,6,12,6);
    hlayout->addStretch(1);
    hlayout->addWidget(qrBtn);

    QLabel *dateCodeLabel = new QLabel("质保期");

    m_dateCodeEdit->setFixedSize(584,36);

    QWidget *btnGroup = new QWidget();
    QHBoxLayout *btnHLayout = new QHBoxLayout(btnGroup);
    QString activateText = m_isActivated ? QString("重新激活") : QString("激活");
    m_activateBtn = new QPushButton(activateText);
    m_activateBtn->setFixedSize(110,40);
    QPushButton *cancelBtn = new QPushButton("取消");
    cancelBtn->setFixedSize(110,40);
    m_activateBtn->setStyleSheet("QPushButton{"
                                 "background-color:#2eb3ff;"
                                 "border:none;"
                                 "border-radius:6px;"
                                 "color:#fff;"
                                 "font-size:15px;"
                                 "}"
                                 "QPushButton:hover{"
                                 "background-color:#3fc4ff;"
                                 "border:none;"
                                 "color:#fff;"
                                 "}"
                                 "QPushButton:pressed{"
                                 "background-color:#1da2ff;"
                                 "border:none;"
                                 "color:#fff;"
                                 "}");
    cancelBtn->setStyleSheet("QPushButton{"
                             "background-color:#393939;"
                             "border:none;"
                             "border-radius:6px;"
                             "color:#fff;"
                             "font-size:15px;"
                             "}"
                             "QPushButton:hover{"
                             "background-color:#464646;"
                             "border:none;"
                             "color:#fff;"
                             "}"
                             "QPushButton:pressed{"
                             "background-color:#343434;"
                             "border:none;"
                             "color:#fff;"
                             "}");
    btnHLayout->addStretch();
    btnHLayout->addWidget(m_activateBtn);
    btnHLayout->addSpacing(36);
    btnHLayout->addWidget(cancelBtn);
    btnHLayout->addStretch();


    mainVLayout->addSpacing(16);
    mainVLayout->addWidget(licenseCodeLabel);
    mainVLayout->addSpacing(4);
    mainVLayout->addWidget(m_licenseCodeEdit);
    mainVLayout->addStretch();
    mainVLayout->addWidget(machineCodeLabel);
    mainVLayout->addSpacing(4);
    mainVLayout->addWidget(machineCodeEdit);
    mainVLayout->addStretch();
    mainVLayout->addWidget(dateCodeLabel);
    mainVLayout->addSpacing(4);
    mainVLayout->addWidget(m_dateCodeEdit);
    mainVLayout->addSpacing(5);
    mainVLayout->addStretch();
    mainVLayout->addWidget(btnGroup);
    mainVLayout->addSpacing(20);

    if (m_isActivated && !m_activateCode.isEmpty()){
        m_licenseCodeEdit->setText(m_activateCode);
        m_licenseCodeEdit->setDisabled(true);
    }

    QString machineCodeText = !m_machineCode.isEmpty() ? m_machineCode : QString("查询失败");
    machineCodeEdit->setText(machineCodeText);

    QString expiredText = !m_expiredDate.isEmpty() ? m_expiredDate : QString("查询失败");
    m_dateCodeEdit->setText(expiredText);
    m_dateCodeEdit->setDisabled(true);

    connect(m_activateBtn, SIGNAL(clicked()), this, SLOT(acceptBtnClicked()));
    connect(cancelBtn, SIGNAL(clicked()), this, SLOT(close()));
}

void ActivatePage::getLicenseInfo()
{
    LicenseEntry::instance().getLicenseInfo(m_isActivated, m_machineCode, m_activateCode, m_expiredDate);
    // 更新质保期
    QString expiredText = !m_expiredDate.isEmpty() ? m_expiredDate : QString("查询失败");
    m_dateCodeEdit->setText(expiredText);
    //    KLOG_INFO() << "a:" << m_isActivated << "mc:" << m_machineCode << "ac:" << m_activateCode << "ed:" << m_expiredDate;
}

void ActivatePage::acceptBtnClicked()
{
    //激活和重新激活复用一个按钮
    if(m_isActivated && !m_activateCode.isEmpty() && QString::compare(m_activateBtn->text(),QString("重新激活"))==0){
        m_activateBtn->setText("激活");
        m_licenseCodeEdit->setDisabled(false);
        return;
    }
    QString activeCode = m_licenseCodeEdit->text();
    // 激活完后重新取结果, 更新全局变量
    bool ret = LicenseEntry::instance().activation(activeCode);
    getLicenseInfo();
    if (m_isActivated && !m_activateCode.isEmpty()){
        // 未激活状态不会进这里
        m_licenseCodeEdit->setText(m_activateCode);
        m_licenseCodeEdit->setDisabled(true);
        m_activateBtn->setText("重新激活");
    }
    if (ret){
        Dialog *retDialog = new Dialog(this,"activate");
        retDialog->exec();
        this->close();
    }else{
        Dialog *retDialog = new Dialog(this,"activateFailed");
        retDialog->exec();
    }
}

void ActivatePage::showQR()
{
    Dialog *qrDialog = new Dialog(this, "qrcode");
    QLabel *qrCodeLabel = qrDialog->getQrLabel();
    qrCodeLabel->setStyleSheet("image:url(:/images/128px.png)");
    qrCodeLabel->setAlignment(Qt::AlignCenter);
    genQRcode(qrCodeLabel);
    qrDialog->exec();
}

void ActivatePage::genQRcode(QLabel *l)
{
    int height = 160;
    int width = 160;
    QPixmap qrPix;
    QRcode *qrcode = QRcode_encodeString(m_machineCode.toStdString().c_str(), 2, QR_ECLEVEL_Q, QR_MODE_8, 1);
    int qrcodeWidth = qrcode->width > 0 ? qrcode->width : 1;
    double scaledWidth = (double)width / (double)qrcodeWidth;
    double scaledHeight = (double)height / (double)qrcodeWidth;
    QImage qrimage = QImage(width, height, QImage::Format_ARGB32);
    QPainter qrpainter(&qrimage);
    QColor background(Qt::white);
    qrpainter.setBrush(background);
    qrpainter.setPen(Qt::NoPen);
    qrpainter.drawRect(0,0,width,height);
    QColor foreground(Qt::black);
    qrpainter.setBrush(foreground);

    for(qint32 y = 0; y < qrcodeWidth; y++){
        for(qint32 x = 0; x < qrcodeWidth; x++){
            unsigned char b = qrcode->data[y * qrcodeWidth + x];
            if (b & 0x01){
                QRectF r(x * scaledWidth, y * scaledHeight, scaledWidth, scaledHeight);
                qrpainter.drawRects(&r, 1);
            }
        }
    }
    qrPix = QPixmap::fromImage(qrimage);
    l->setPixmap(qrPix);

}
