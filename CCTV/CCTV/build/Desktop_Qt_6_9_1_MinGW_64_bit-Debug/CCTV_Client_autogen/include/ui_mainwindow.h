/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.9.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QAction *actionSave_Settings;
    QAction *actionLoad_Settings;
    QAction *actionExit;
    QAction *actionFullscreen;
    QAction *actionShow_Log;
    QAction *actionAbout;
    QAction *actionUser_Manual;
    QWidget *centralwidget;
    QVBoxLayout *mainVerticalLayout;
    QFrame *headerFrame;
    QHBoxLayout *headerLayout;
    QLabel *titleLabel;
    QSpacerItem *headerSpacer;
    QLabel *connectionStatusIcon;
    QLabel *dateTimeLabel;
    QTabWidget *mainTabWidget;
    QWidget *networkTab;
    QVBoxLayout *networkTabLayout;
    QGroupBox *connectionGroupBox;
    QGridLayout *connectionGridLayout;
    QLabel *hostLabel;
    QLineEdit *hostLineEdit;
    QLabel *portLabel;
    QSpinBox *portSpinBox;
    QHBoxLayout *connectionButtonLayout;
    QPushButton *connectButton;
    QPushButton *disconnectButton;
    QLabel *connectionStatusLabel;
    QGroupBox *networkStatsGroupBox;
    QGridLayout *statsGridLayout;
    QLabel *pingLabel;
    QLabel *pingValueLabel;
    QLabel *dataRateLabel;
    QLabel *dataRateValueLabel;
    QSpacerItem *networkTabSpacer;
    QWidget *videoTab;
    QVBoxLayout *videoTabLayout;
    QFrame *videoFrame;
    QVBoxLayout *videoFrameLayout;
    QLabel *videoLabel;
    QGroupBox *videoControlGroupBox;
    QHBoxLayout *videoControlLayout;
    QPushButton *startStreamButton;
    QPushButton *stopStreamButton;
    QSpacerItem *videoControlSpacer;
    QLabel *videoStatusLabel;
    QLabel *fpsLabel;
    QWidget *imageTab;
    QVBoxLayout *imageTabLayout;
    QLabel *imageTabPlaceholder;
    QWidget *controlTab;
    QVBoxLayout *controlTabLayout;
    QGroupBox *captureGroupBox;
    QVBoxLayout *captureLayout;
    QPushButton *captureButton;
    QGroupBox *lcdGroupBox;
    QGridLayout *lcdGridLayout;
    QPushButton *lcd1Button;
    QPushButton *lcd2Button;
    QPushButton *lcd3Button;
    QPushButton *lcd4Button;
    QSpacerItem *controlTabSpacer;
    QGroupBox *logGroupBox;
    QVBoxLayout *logLayout;
    QTextEdit *logTextEdit;
    QMenuBar *menubar;
    QMenu *menuFile;
    QMenu *menuView;
    QMenu *menuHelp;
    QStatusBar *statusbar;
    QLabel *statusBarLabel;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(1200, 800);
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/icons/camera.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        MainWindow->setWindowIcon(icon);
        MainWindow->setStyleSheet(QString::fromUtf8("/* \353\251\224\354\235\270 \354\234\210\353\217\204\354\232\260 \354\212\244\355\203\200\354\235\274 */\n"
"QMainWindow {\n"
"    background-color: #2b2b2b;\n"
"    color: #ffffff;\n"
"}\n"
"\n"
"/* \355\203\255 \354\234\204\354\240\257 \354\212\244\355\203\200\354\235\274 */\n"
"QTabWidget::pane {\n"
"    border: 2px solid #555555;\n"
"    background-color: #3c3c3c;\n"
"    border-radius: 8px;\n"
"}\n"
"\n"
"QTabWidget::tab-bar {\n"
"    alignment: center;\n"
"}\n"
"\n"
"QTabBar::tab {\n"
"    background-color: #555555;\n"
"    color: #ffffff;\n"
"    padding: 12px 20px;\n"
"    margin: 2px;\n"
"    border-radius: 6px;\n"
"    font-weight: bold;\n"
"    font-size: 11px;\n"
"}\n"
"\n"
"QTabBar::tab:selected {\n"
"    background-color: #0078d4;\n"
"    color: #ffffff;\n"
"}\n"
"\n"
"QTabBar::tab:hover {\n"
"    background-color: #666666;\n"
"}\n"
"\n"
"/* \352\267\270\353\243\271\353\260\225\354\212\244 \354\212\244\355\203\200\354\235\274 */\n"
"QGroupBox {\n"
"    font-weight: bold;\n"
"    font-size: 12px;\n"
""
                        "    color: #ffffff;\n"
"    border: 2px solid #555555;\n"
"    border-radius: 8px;\n"
"    margin-top: 10px;\n"
"    padding-top: 10px;\n"
"}\n"
"\n"
"QGroupBox::title {\n"
"    subcontrol-origin: margin;\n"
"    left: 10px;\n"
"    padding: 0 8px 0 8px;\n"
"    color: #0078d4;\n"
"}\n"
"\n"
"/* \353\262\204\355\212\274 \354\212\244\355\203\200\354\235\274 */\n"
"QPushButton {\n"
"    background-color: #0078d4;\n"
"    border: none;\n"
"    color: white;\n"
"    padding: 10px 16px;\n"
"    font-size: 11px;\n"
"    font-weight: bold;\n"
"    border-radius: 6px;\n"
"    min-height: 25px;\n"
"}\n"
"\n"
"QPushButton:hover {\n"
"    background-color: #106ebe;\n"
"}\n"
"\n"
"QPushButton:pressed {\n"
"    background-color: #005a9e;\n"
"}\n"
"\n"
"QPushButton:disabled {\n"
"    background-color: #666666;\n"
"    color: #999999;\n"
"}\n"
"\n"
"/* \355\212\271\353\263\204\355\225\234 \353\262\204\355\212\274\353\223\244 */\n"
"QPushButton#connectButton {\n"
"    background-color: #107c10;\n"
"}\n"
"\n"
"QPushButton#conn"
                        "ectButton:hover {\n"
"    background-color: #0e6e0e;\n"
"}\n"
"\n"
"QPushButton#disconnectButton {\n"
"    background-color: #d13438;\n"
"}\n"
"\n"
"QPushButton#disconnectButton:hover {\n"
"    background-color: #b52d32;\n"
"}\n"
"\n"
"/* LCD \354\240\234\354\226\264 \353\262\204\355\212\274\353\223\244 */\n"
"QPushButton[objectName*=\"lcd\"] {\n"
"    background-color: #ff8c00;\n"
"    min-height: 40px;\n"
"    font-size: 12px;\n"
"}\n"
"\n"
"QPushButton[objectName*=\"lcd\"]:hover {\n"
"    background-color: #ff7700;\n"
"}\n"
"\n"
"/* \354\236\205\353\240\245 \355\225\204\353\223\234 \354\212\244\355\203\200\354\235\274 */\n"
"QLineEdit, QSpinBox {\n"
"    background-color: #4c4c4c;\n"
"    border: 2px solid #666666;\n"
"    border-radius: 4px;\n"
"    padding: 8px;\n"
"    color: #ffffff;\n"
"    font-size: 11px;\n"
"}\n"
"\n"
"QLineEdit:focus, QSpinBox:focus {\n"
"    border-color: #0078d4;\n"
"}\n"
"\n"
"/* \353\235\274\353\262\250 \354\212\244\355\203\200\354\235\274 */\n"
"QLabel {\n"
"    color: #ffffff"
                        ";\n"
"    font-size: 11px;\n"
"}\n"
"\n"
"QLabel[objectName*=\"status\"] {\n"
"    font-weight: bold;\n"
"    font-size: 12px;\n"
"}\n"
"\n"
"QLabel[objectName*=\"title\"] {\n"
"    font-size: 14px;\n"
"    font-weight: bold;\n"
"    color: #0078d4;\n"
"}\n"
"\n"
"/* \355\205\215\354\212\244\355\212\270 \354\227\220\353\224\224\355\212\270 (\353\241\234\352\267\270) \354\212\244\355\203\200\354\235\274 */\n"
"QTextEdit {\n"
"    background-color: #1e1e1e;\n"
"    border: 2px solid #555555;\n"
"    border-radius: 6px;\n"
"    color: #ffffff;\n"
"    font-family: 'Consolas', 'Monaco', monospace;\n"
"    font-size: 10px;\n"
"}\n"
"\n"
"/* \354\212\254\353\235\274\354\235\264\353\215\224 \354\212\244\355\203\200\354\235\274 */\n"
"QSlider::groove:horizontal {\n"
"    border: 1px solid #999999;\n"
"    height: 8px;\n"
"    background: #4c4c4c;\n"
"    border-radius: 4px;\n"
"}\n"
"\n"
"QSlider::handle:horizontal {\n"
"    background: #0078d4;\n"
"    border: 1px solid #5c5c5c;\n"
"    width: 18px;\n"
"    margin: -"
                        "2px 0;\n"
"    border-radius: 9px;\n"
"}\n"
"\n"
"QSlider::handle:horizontal:hover {\n"
"    background: #106ebe;\n"
"}\n"
"\n"
"/* \354\212\244\355\201\254\353\241\244 \354\230\201\354\227\255 \354\212\244\355\203\200\354\235\274 */\n"
"QScrollArea {\n"
"    border: 2px solid #555555;\n"
"    border-radius: 6px;\n"
"    background-color: #2b2b2b;\n"
"}\n"
"\n"
"QScrollBar:vertical {\n"
"    background: #3c3c3c;\n"
"    width: 12px;\n"
"    border-radius: 6px;\n"
"}\n"
"\n"
"QScrollBar::handle:vertical {\n"
"    background: #0078d4;\n"
"    border-radius: 6px;\n"
"    min-height: 20px;\n"
"}\n"
"\n"
"QScrollBar::handle:vertical:hover {\n"
"    background: #106ebe;\n"
"}"));
        actionSave_Settings = new QAction(MainWindow);
        actionSave_Settings->setObjectName("actionSave_Settings");
        actionLoad_Settings = new QAction(MainWindow);
        actionLoad_Settings->setObjectName("actionLoad_Settings");
        actionExit = new QAction(MainWindow);
        actionExit->setObjectName("actionExit");
        actionFullscreen = new QAction(MainWindow);
        actionFullscreen->setObjectName("actionFullscreen");
        actionShow_Log = new QAction(MainWindow);
        actionShow_Log->setObjectName("actionShow_Log");
        actionAbout = new QAction(MainWindow);
        actionAbout->setObjectName("actionAbout");
        actionUser_Manual = new QAction(MainWindow);
        actionUser_Manual->setObjectName("actionUser_Manual");
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        mainVerticalLayout = new QVBoxLayout(centralwidget);
        mainVerticalLayout->setSpacing(10);
        mainVerticalLayout->setObjectName("mainVerticalLayout");
        mainVerticalLayout->setContentsMargins(15, 15, 15, 10);
        headerFrame = new QFrame(centralwidget);
        headerFrame->setObjectName("headerFrame");
        headerFrame->setStyleSheet(QString::fromUtf8("QFrame#headerFrame {\n"
"    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,\n"
"                               stop: 0 #0078d4, stop: 1 #005a9e);\n"
"    border-radius: 10px;\n"
"    border: 2px solid #004578;\n"
"}"));
        headerFrame->setFrameShape(QFrame::Shape::StyledPanel);
        headerLayout = new QHBoxLayout(headerFrame);
        headerLayout->setObjectName("headerLayout");
        headerLayout->setContentsMargins(20, 15, 20, 15);
        titleLabel = new QLabel(headerFrame);
        titleLabel->setObjectName("titleLabel");
        titleLabel->setStyleSheet(QString::fromUtf8("QLabel {\n"
"    color: #ffffff;\n"
"    font-size: 18px;\n"
"    font-weight: bold;\n"
"}"));

        headerLayout->addWidget(titleLabel);

        headerSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        headerLayout->addItem(headerSpacer);

        connectionStatusIcon = new QLabel(headerFrame);
        connectionStatusIcon->setObjectName("connectionStatusIcon");
        connectionStatusIcon->setStyleSheet(QString::fromUtf8("font-size: 16px;"));

        headerLayout->addWidget(connectionStatusIcon);

        dateTimeLabel = new QLabel(headerFrame);
        dateTimeLabel->setObjectName("dateTimeLabel");
        dateTimeLabel->setStyleSheet(QString::fromUtf8("QLabel {\n"
"    color: #ffffff;\n"
"    font-size: 14px;\n"
"    font-weight: bold;\n"
"    font-family: 'Courier New', monospace;\n"
"}"));

        headerLayout->addWidget(dateTimeLabel);


        mainVerticalLayout->addWidget(headerFrame);

        mainTabWidget = new QTabWidget(centralwidget);
        mainTabWidget->setObjectName("mainTabWidget");
        networkTab = new QWidget();
        networkTab->setObjectName("networkTab");
        networkTabLayout = new QVBoxLayout(networkTab);
        networkTabLayout->setSpacing(15);
        networkTabLayout->setObjectName("networkTabLayout");
        connectionGroupBox = new QGroupBox(networkTab);
        connectionGroupBox->setObjectName("connectionGroupBox");
        connectionGridLayout = new QGridLayout(connectionGroupBox);
        connectionGridLayout->setObjectName("connectionGridLayout");
        connectionGridLayout->setHorizontalSpacing(15);
        connectionGridLayout->setVerticalSpacing(10);
        hostLabel = new QLabel(connectionGroupBox);
        hostLabel->setObjectName("hostLabel");
        hostLabel->setStyleSheet(QString::fromUtf8("font-weight: bold;"));

        connectionGridLayout->addWidget(hostLabel, 0, 0, 1, 1);

        hostLineEdit = new QLineEdit(connectionGroupBox);
        hostLineEdit->setObjectName("hostLineEdit");

        connectionGridLayout->addWidget(hostLineEdit, 0, 1, 1, 1);

        portLabel = new QLabel(connectionGroupBox);
        portLabel->setObjectName("portLabel");
        portLabel->setStyleSheet(QString::fromUtf8("font-weight: bold;"));

        connectionGridLayout->addWidget(portLabel, 1, 0, 1, 1);

        portSpinBox = new QSpinBox(connectionGroupBox);
        portSpinBox->setObjectName("portSpinBox");
        portSpinBox->setMinimum(1);
        portSpinBox->setMaximum(65535);
        portSpinBox->setValue(8888);

        connectionGridLayout->addWidget(portSpinBox, 1, 1, 1, 1);

        connectionButtonLayout = new QHBoxLayout();
        connectionButtonLayout->setObjectName("connectionButtonLayout");
        connectButton = new QPushButton(connectionGroupBox);
        connectButton->setObjectName("connectButton");

        connectionButtonLayout->addWidget(connectButton);

        disconnectButton = new QPushButton(connectionGroupBox);
        disconnectButton->setObjectName("disconnectButton");
        disconnectButton->setEnabled(false);

        connectionButtonLayout->addWidget(disconnectButton);


        connectionGridLayout->addLayout(connectionButtonLayout, 2, 0, 1, 2);

        connectionStatusLabel = new QLabel(connectionGroupBox);
        connectionStatusLabel->setObjectName("connectionStatusLabel");
        connectionStatusLabel->setStyleSheet(QString::fromUtf8("QLabel {\n"
"    color: #d13438;\n"
"    font-weight: bold;\n"
"    font-size: 13px;\n"
"    padding: 8px;\n"
"    background-color: rgba(209, 52, 56, 0.1);\n"
"    border-radius: 4px;\n"
"}"));
        connectionStatusLabel->setAlignment(Qt::AlignmentFlag::AlignCenter);

        connectionGridLayout->addWidget(connectionStatusLabel, 3, 0, 1, 2);


        networkTabLayout->addWidget(connectionGroupBox);

        networkStatsGroupBox = new QGroupBox(networkTab);
        networkStatsGroupBox->setObjectName("networkStatsGroupBox");
        statsGridLayout = new QGridLayout(networkStatsGroupBox);
        statsGridLayout->setObjectName("statsGridLayout");
        pingLabel = new QLabel(networkStatsGroupBox);
        pingLabel->setObjectName("pingLabel");

        statsGridLayout->addWidget(pingLabel, 0, 0, 1, 1);

        pingValueLabel = new QLabel(networkStatsGroupBox);
        pingValueLabel->setObjectName("pingValueLabel");
        pingValueLabel->setStyleSheet(QString::fromUtf8("color: #00ff00; font-weight: bold;"));

        statsGridLayout->addWidget(pingValueLabel, 0, 1, 1, 1);

        dataRateLabel = new QLabel(networkStatsGroupBox);
        dataRateLabel->setObjectName("dataRateLabel");

        statsGridLayout->addWidget(dataRateLabel, 1, 0, 1, 1);

        dataRateValueLabel = new QLabel(networkStatsGroupBox);
        dataRateValueLabel->setObjectName("dataRateValueLabel");
        dataRateValueLabel->setStyleSheet(QString::fromUtf8("color: #00ff00; font-weight: bold;"));

        statsGridLayout->addWidget(dataRateValueLabel, 1, 1, 1, 1);


        networkTabLayout->addWidget(networkStatsGroupBox);

        networkTabSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        networkTabLayout->addItem(networkTabSpacer);

        mainTabWidget->addTab(networkTab, QString());
        videoTab = new QWidget();
        videoTab->setObjectName("videoTab");
        videoTabLayout = new QVBoxLayout(videoTab);
        videoTabLayout->setObjectName("videoTabLayout");
        videoFrame = new QFrame(videoTab);
        videoFrame->setObjectName("videoFrame");
        videoFrame->setStyleSheet(QString::fromUtf8("QFrame#videoFrame {\n"
"    background-color: #000000;\n"
"    border: 3px solid #555555;\n"
"    border-radius: 10px;\n"
"}"));
        videoFrame->setFrameShape(QFrame::Shape::StyledPanel);
        videoFrameLayout = new QVBoxLayout(videoFrame);
        videoFrameLayout->setObjectName("videoFrameLayout");
        videoLabel = new QLabel(videoFrame);
        videoLabel->setObjectName("videoLabel");
        videoLabel->setStyleSheet(QString::fromUtf8("QLabel {\n"
"    color: #888888;\n"
"    font-size: 16px;\n"
"    font-weight: bold;\n"
"    background-color: transparent;\n"
"}"));
        videoLabel->setScaledContents(true);
        videoLabel->setAlignment(Qt::AlignmentFlag::AlignCenter);

        videoFrameLayout->addWidget(videoLabel);


        videoTabLayout->addWidget(videoFrame);

        videoControlGroupBox = new QGroupBox(videoTab);
        videoControlGroupBox->setObjectName("videoControlGroupBox");
        videoControlLayout = new QHBoxLayout(videoControlGroupBox);
        videoControlLayout->setObjectName("videoControlLayout");
        startStreamButton = new QPushButton(videoControlGroupBox);
        startStreamButton->setObjectName("startStreamButton");
        startStreamButton->setStyleSheet(QString::fromUtf8("QPushButton {\n"
"    background-color: #107c10;\n"
"    min-height: 35px;\n"
"    font-size: 12px;\n"
"}\n"
"QPushButton:hover {\n"
"    background-color: #0e6e0e;\n"
"}"));

        videoControlLayout->addWidget(startStreamButton);

        stopStreamButton = new QPushButton(videoControlGroupBox);
        stopStreamButton->setObjectName("stopStreamButton");
        stopStreamButton->setEnabled(false);
        stopStreamButton->setStyleSheet(QString::fromUtf8("QPushButton {\n"
"    background-color: #d13438;\n"
"    min-height: 35px;\n"
"    font-size: 12px;\n"
"}\n"
"QPushButton:hover {\n"
"    background-color: #b52d32;\n"
"}"));

        videoControlLayout->addWidget(stopStreamButton);

        videoControlSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        videoControlLayout->addItem(videoControlSpacer);

        videoStatusLabel = new QLabel(videoControlGroupBox);
        videoStatusLabel->setObjectName("videoStatusLabel");
        videoStatusLabel->setStyleSheet(QString::fromUtf8("QLabel {\n"
"    color: #d13438;\n"
"    font-weight: bold;\n"
"    padding: 8px;\n"
"    background-color: rgba(209, 52, 56, 0.1);\n"
"    border-radius: 4px;\n"
"}"));

        videoControlLayout->addWidget(videoStatusLabel);

        fpsLabel = new QLabel(videoControlGroupBox);
        fpsLabel->setObjectName("fpsLabel");
        fpsLabel->setStyleSheet(QString::fromUtf8("QLabel {\n"
"    color: #0078d4;\n"
"    font-weight: bold;\n"
"    padding: 8px;\n"
"    background-color: rgba(0, 120, 212, 0.1);\n"
"    border-radius: 4px;\n"
"}"));

        videoControlLayout->addWidget(fpsLabel);


        videoTabLayout->addWidget(videoControlGroupBox);

        mainTabWidget->addTab(videoTab, QString());
        imageTab = new QWidget();
        imageTab->setObjectName("imageTab");
        imageTabLayout = new QVBoxLayout(imageTab);
        imageTabLayout->setObjectName("imageTabLayout");
        imageTabPlaceholder = new QLabel(imageTab);
        imageTabPlaceholder->setObjectName("imageTabPlaceholder");
        imageTabPlaceholder->setStyleSheet(QString::fromUtf8("color: #888888; font-size: 14px;"));
        imageTabPlaceholder->setAlignment(Qt::AlignmentFlag::AlignCenter);

        imageTabLayout->addWidget(imageTabPlaceholder);

        mainTabWidget->addTab(imageTab, QString());
        controlTab = new QWidget();
        controlTab->setObjectName("controlTab");
        controlTabLayout = new QVBoxLayout(controlTab);
        controlTabLayout->setSpacing(20);
        controlTabLayout->setObjectName("controlTabLayout");
        captureGroupBox = new QGroupBox(controlTab);
        captureGroupBox->setObjectName("captureGroupBox");
        captureLayout = new QVBoxLayout(captureGroupBox);
        captureLayout->setObjectName("captureLayout");
        captureButton = new QPushButton(captureGroupBox);
        captureButton->setObjectName("captureButton");
        captureButton->setStyleSheet(QString::fromUtf8("QPushButton {\n"
"    background-color: #8a2be2;\n"
"    min-height: 45px;\n"
"    font-size: 14px;\n"
"    font-weight: bold;\n"
"}\n"
"QPushButton:hover {\n"
"    background-color: #7b1fa2;\n"
"}"));

        captureLayout->addWidget(captureButton);


        controlTabLayout->addWidget(captureGroupBox);

        lcdGroupBox = new QGroupBox(controlTab);
        lcdGroupBox->setObjectName("lcdGroupBox");
        lcdGridLayout = new QGridLayout(lcdGroupBox);
        lcdGridLayout->setSpacing(15);
        lcdGridLayout->setObjectName("lcdGridLayout");
        lcd1Button = new QPushButton(lcdGroupBox);
        lcd1Button->setObjectName("lcd1Button");
        lcd1Button->setStyleSheet(QString::fromUtf8("QPushButton {\n"
"    background-color: #ff6b35;\n"
"    min-height: 50px;\n"
"    font-size: 13px;\n"
"    font-weight: bold;\n"
"    border-radius: 8px;\n"
"}\n"
"QPushButton:hover {\n"
"    background-color: #e55a2b;\n"
"}"));

        lcdGridLayout->addWidget(lcd1Button, 0, 0, 1, 1);

        lcd2Button = new QPushButton(lcdGroupBox);
        lcd2Button->setObjectName("lcd2Button");
        lcd2Button->setStyleSheet(QString::fromUtf8("QPushButton {\n"
"    background-color: #ff6b35;\n"
"    min-height: 50px;\n"
"    font-size: 13px;\n"
"    font-weight: bold;\n"
"    border-radius: 8px;\n"
"}\n"
"QPushButton:hover {\n"
"    background-color: #e55a2b;\n"
"}"));

        lcdGridLayout->addWidget(lcd2Button, 0, 1, 1, 1);

        lcd3Button = new QPushButton(lcdGroupBox);
        lcd3Button->setObjectName("lcd3Button");
        lcd3Button->setStyleSheet(QString::fromUtf8("QPushButton {\n"
"    background-color: #ff6b35;\n"
"    min-height: 50px;\n"
"    font-size: 13px;\n"
"    font-weight: bold;\n"
"    border-radius: 8px;\n"
"}\n"
"QPushButton:hover {\n"
"    background-color: #e55a2b;\n"
"}"));

        lcdGridLayout->addWidget(lcd3Button, 1, 0, 1, 1);

        lcd4Button = new QPushButton(lcdGroupBox);
        lcd4Button->setObjectName("lcd4Button");
        lcd4Button->setStyleSheet(QString::fromUtf8("QPushButton {\n"
"    background-color: #ff6b35;\n"
"    min-height: 50px;\n"
"    font-size: 13px;\n"
"    font-weight: bold;\n"
"    border-radius: 8px;\n"
"}\n"
"QPushButton:hover {\n"
"    background-color: #e55a2b;\n"
"}"));

        lcdGridLayout->addWidget(lcd4Button, 1, 1, 1, 1);


        controlTabLayout->addWidget(lcdGroupBox);

        controlTabSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        controlTabLayout->addItem(controlTabSpacer);

        mainTabWidget->addTab(controlTab, QString());

        mainVerticalLayout->addWidget(mainTabWidget);

        logGroupBox = new QGroupBox(centralwidget);
        logGroupBox->setObjectName("logGroupBox");
        logLayout = new QVBoxLayout(logGroupBox);
        logLayout->setObjectName("logLayout");
        logTextEdit = new QTextEdit(logGroupBox);
        logTextEdit->setObjectName("logTextEdit");
        logTextEdit->setReadOnly(true);

        logLayout->addWidget(logTextEdit);


        mainVerticalLayout->addWidget(logGroupBox);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 1200, 26));
        menuFile = new QMenu(menubar);
        menuFile->setObjectName("menuFile");
        menuView = new QMenu(menubar);
        menuView->setObjectName("menuView");
        menuHelp = new QMenu(menubar);
        menuHelp->setObjectName("menuHelp");
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        statusBarLabel = new QLabel(statusbar);
        statusBarLabel->setObjectName("statusBarLabel");
        statusBarLabel->setGeometry(QRect(0, 0, 100, 30));
        MainWindow->setStatusBar(statusbar);

        menubar->addAction(menuFile->menuAction());
        menubar->addAction(menuView->menuAction());
        menubar->addAction(menuHelp->menuAction());
        menuFile->addAction(actionSave_Settings);
        menuFile->addAction(actionLoad_Settings);
        menuFile->addSeparator();
        menuFile->addAction(actionExit);
        menuView->addAction(actionFullscreen);
        menuView->addAction(actionShow_Log);
        menuHelp->addAction(actionAbout);
        menuHelp->addAction(actionUser_Manual);

        retranslateUi(MainWindow);

        mainTabWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "CCTV Remote Client - Professional Monitoring System", nullptr));
        actionSave_Settings->setText(QCoreApplication::translate("MainWindow", "Save Settings", nullptr));
#if QT_CONFIG(shortcut)
        actionSave_Settings->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+S", nullptr));
#endif // QT_CONFIG(shortcut)
        actionLoad_Settings->setText(QCoreApplication::translate("MainWindow", "Load Settings", nullptr));
#if QT_CONFIG(shortcut)
        actionLoad_Settings->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+O", nullptr));
#endif // QT_CONFIG(shortcut)
        actionExit->setText(QCoreApplication::translate("MainWindow", "Exit", nullptr));
#if QT_CONFIG(shortcut)
        actionExit->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+Q", nullptr));
#endif // QT_CONFIG(shortcut)
        actionFullscreen->setText(QCoreApplication::translate("MainWindow", "Fullscreen", nullptr));
#if QT_CONFIG(shortcut)
        actionFullscreen->setShortcut(QCoreApplication::translate("MainWindow", "F11", nullptr));
#endif // QT_CONFIG(shortcut)
        actionShow_Log->setText(QCoreApplication::translate("MainWindow", "Show/Hide Log", nullptr));
#if QT_CONFIG(shortcut)
        actionShow_Log->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+L", nullptr));
#endif // QT_CONFIG(shortcut)
        actionAbout->setText(QCoreApplication::translate("MainWindow", "About", nullptr));
        actionUser_Manual->setText(QCoreApplication::translate("MainWindow", "User Manual", nullptr));
#if QT_CONFIG(shortcut)
        actionUser_Manual->setShortcut(QCoreApplication::translate("MainWindow", "F1", nullptr));
#endif // QT_CONFIG(shortcut)
        titleLabel->setText(QCoreApplication::translate("MainWindow", "\360\237\216\245 CCTV Remote Monitoring System", nullptr));
        connectionStatusIcon->setText(QCoreApplication::translate("MainWindow", "\360\237\224\264", nullptr));
        dateTimeLabel->setText(QCoreApplication::translate("MainWindow", "2024-01-01 12:00:00", nullptr));
        connectionGroupBox->setTitle(QCoreApplication::translate("MainWindow", "Server Connection Settings", nullptr));
        hostLabel->setText(QCoreApplication::translate("MainWindow", "\360\237\226\245\357\270\217 Server Address:", nullptr));
        hostLineEdit->setText(QCoreApplication::translate("MainWindow", "192.168.1.100", nullptr));
        hostLineEdit->setPlaceholderText(QCoreApplication::translate("MainWindow", "Enter Raspberry Pi IP address (e.g., 192.168.1.100)", nullptr));
        portLabel->setText(QCoreApplication::translate("MainWindow", "\360\237\224\214 Port Number:", nullptr));
        connectButton->setText(QCoreApplication::translate("MainWindow", "\360\237\224\227 Connect to Server", nullptr));
        disconnectButton->setText(QCoreApplication::translate("MainWindow", "\342\235\214 Disconnect", nullptr));
        connectionStatusLabel->setText(QCoreApplication::translate("MainWindow", "Status: Disconnected", nullptr));
        networkStatsGroupBox->setTitle(QCoreApplication::translate("MainWindow", "Network Statistics", nullptr));
        pingLabel->setText(QCoreApplication::translate("MainWindow", "\360\237\223\241 Ping:", nullptr));
        pingValueLabel->setText(QCoreApplication::translate("MainWindow", "-- ms", nullptr));
        dataRateLabel->setText(QCoreApplication::translate("MainWindow", "\360\237\223\212 Data Rate:", nullptr));
        dataRateValueLabel->setText(QCoreApplication::translate("MainWindow", "-- KB/s", nullptr));
        mainTabWidget->setTabText(mainTabWidget->indexOf(networkTab), QCoreApplication::translate("MainWindow", "\360\237\214\220 Network Connection", nullptr));
        videoLabel->setText(QCoreApplication::translate("MainWindow", "\360\237\223\272 No Video Stream\n"
"Click 'Start Streaming' to begin", nullptr));
        videoControlGroupBox->setTitle(QCoreApplication::translate("MainWindow", "Video Stream Controls", nullptr));
        startStreamButton->setText(QCoreApplication::translate("MainWindow", "\342\226\266\357\270\217 Start Streaming", nullptr));
        stopStreamButton->setText(QCoreApplication::translate("MainWindow", "\342\217\271\357\270\217 Stop Streaming", nullptr));
        videoStatusLabel->setText(QCoreApplication::translate("MainWindow", "Status: Stopped", nullptr));
        fpsLabel->setText(QCoreApplication::translate("MainWindow", "FPS: 0", nullptr));
        mainTabWidget->setTabText(mainTabWidget->indexOf(videoTab), QCoreApplication::translate("MainWindow", "\360\237\223\271 Live Video Stream", nullptr));
        imageTabPlaceholder->setText(QCoreApplication::translate("MainWindow", "\360\237\223\267 Image Viewer will be loaded here programmatically", nullptr));
        mainTabWidget->setTabText(mainTabWidget->indexOf(imageTab), QCoreApplication::translate("MainWindow", "\360\237\226\274\357\270\217 Captured Images", nullptr));
        captureGroupBox->setTitle(QCoreApplication::translate("MainWindow", "Image Capture Control", nullptr));
        captureButton->setText(QCoreApplication::translate("MainWindow", "\360\237\223\270 Capture Image from Server", nullptr));
        lcdGroupBox->setTitle(QCoreApplication::translate("MainWindow", "LCD Display Control Panel", nullptr));
        lcd1Button->setText(QCoreApplication::translate("MainWindow", "\360\237\222\241 LCD 1 (OFF)", nullptr));
        lcd2Button->setText(QCoreApplication::translate("MainWindow", "\360\237\222\241 LCD 2 (OFF)", nullptr));
        lcd3Button->setText(QCoreApplication::translate("MainWindow", "\360\237\222\241 LCD 3 (OFF)", nullptr));
        lcd4Button->setText(QCoreApplication::translate("MainWindow", "\360\237\222\241 LCD 4 (OFF)", nullptr));
        mainTabWidget->setTabText(mainTabWidget->indexOf(controlTab), QCoreApplication::translate("MainWindow", "\360\237\216\233\357\270\217 Remote Control", nullptr));
        logGroupBox->setTitle(QCoreApplication::translate("MainWindow", "\360\237\223\213 System Activity Log", nullptr));
        logTextEdit->setHtml(QCoreApplication::translate("MainWindow", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><meta charset=\"utf-8\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"hr { height: 1px; border-width: 0; }\n"
"li.unchecked::marker { content: \"\\2610\"; }\n"
"li.checked::marker { content: \"\\2612\"; }\n"
"</style></head><body style=\" font-family:'Consolas','Monaco','monospace'; font-size:10px; font-weight:400; font-style:normal;\">\n"
"<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" color:#00ff00;\">[12:00:00] CCTV Client initialized successfully</span></p>\n"
"<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" color:#ffff00;\">[12:00:01] Waiting for server connection...</span></p></body></html>", nullptr));
        menuFile->setTitle(QCoreApplication::translate("MainWindow", "File", nullptr));
        menuView->setTitle(QCoreApplication::translate("MainWindow", "View", nullptr));
        menuHelp->setTitle(QCoreApplication::translate("MainWindow", "Help", nullptr));
        statusBarLabel->setText(QCoreApplication::translate("MainWindow", "Ready", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
