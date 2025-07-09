#include "MainWindow.h"
#include "LineDrawingDialog.h"
#include "NetworkConfigDialog.h"
#include <QApplication>
#include <QMessageBox>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QScreen>
#include <QGuiApplication>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QComboBox>
#include <QCalendarWidget>
#include <QDialog>

// ClickableImageLabel 구현
ClickableImageLabel::ClickableImageLabel(QWidget *parent)
    : QLabel(parent)
{
    setCursor(Qt::PointingHandCursor);
    setStyleSheet("border: 2px solid #ddd; border-radius: 8px; padding: 5px; background-color: white;");
}

void ClickableImageLabel::setImageData(const QString &imagePath, const QString &timestamp, const QString &logText)
{
    m_imagePath = imagePath;
    m_timestamp = timestamp;
    m_logText = logText;
}

void ClickableImageLabel::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit clicked(m_imagePath, m_timestamp, m_logText);
    }
    QLabel::mousePressEvent(event);
}

// MainWindow 구현
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_centralWidget(nullptr)
    , m_tabWidget(nullptr)
    , m_liveVideoTab(nullptr)
    , m_videoStreamWidget(nullptr)
    , m_streamingButton(nullptr)
    , m_capturedImageTab(nullptr)
    , m_imageScrollArea(nullptr)
    , m_imageGridWidget(nullptr)
    , m_imageGridLayout(nullptr)
    , m_dateEdit(nullptr)
    , m_hourSpinBox(nullptr)
    , m_dateButton(nullptr)
    , m_calendarWidget(nullptr)
    , m_calendarDialog(nullptr)
    , m_hourComboBox(nullptr)
    , m_requestButton(nullptr)
    , m_statusLabel(nullptr)
    , m_modeComboBox(nullptr)
    , m_networkButton(nullptr)
    , m_rtspUrl("rtsp://192.168.0.81:8554/retransmit")
    , m_tcpHost("192.168.0.81")
    , m_tcpPort(8080)
    , m_isConnected(false)
    , m_tcpCommunicator(nullptr)
    , m_networkManager(nullptr)
    , m_updateTimer(nullptr)
    , m_requestTimeoutTimer(nullptr)
    , m_imageViewerDialog(nullptr)
    , m_networkDialog(nullptr)
    , m_lineDrawingDialog(nullptr)
{
    // 경고 상태 초기화 (4개 모두 OFF)
    m_warningStates = {false, false, false, false};

    // 선택된 날짜 초기화
    m_selectedDate = QDate::currentDate();

    // UI 설정
    setupUI();

    // 네트워크 연결 설정
    setupNetworkConnection();

    // 스타일 적용
    applyStyles();

    // 화면 크기 가져오기
    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->availableGeometry();

    // 화면의 80%로 설정
    int maxWidth = screenGeometry.width() * 0.8;
    int maxHeight = screenGeometry.height() * 0.8;

    setMinimumSize(800, 600);
    resize(maxWidth, maxHeight);

    // 화면 중앙에 배치
    move((screenGeometry.width() - maxWidth) / 2,
         (screenGeometry.height() - maxHeight) / 2);
}

MainWindow::~MainWindow()
{
    if (m_updateTimer) {
        m_updateTimer->stop();
    }
    if (m_requestTimeoutTimer) {
        m_requestTimeoutTimer->stop();
    }
    if (m_calendarDialog) {
        delete m_calendarDialog;
    }
}

void MainWindow::setupUI()
{
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(m_centralWidget);

    // 헤더 영역
    QHBoxLayout *headerLayout = new QHBoxLayout();

    QLabel *titleLabel = new QLabel("CCTV Remote Monitoring System");
    titleLabel->setStyleSheet("color: #F37321; font-size: 24px; font-weight: bold;");
    headerLayout->addWidget(titleLabel);

    headerLayout->addStretch();

    m_networkButton = new QPushButton();
    m_networkButton->setIcon(QIcon(":/CCTVMonitoringSystem/icons/NetworkConnect.png"));
    m_networkButton->setIconSize(QSize(24, 24));
    m_networkButton->setStyleSheet("QPushButton { background-color: transparent; color: white; font-size: 20px; border: none; } QPushButton:hover { background-color: rgba(255,255,255,0.1); border-radius: 20px; }");
    connect(m_networkButton, &QPushButton::clicked, this, &MainWindow::onNetworkConfigClicked);
    headerLayout->addWidget(m_networkButton);

    mainLayout->addLayout(headerLayout);

    // 콘텐츠 영역
    QHBoxLayout *contentLayout = new QHBoxLayout();

    m_tabWidget = new QTabWidget();
    m_tabWidget->setStyleSheet("QTabWidget::pane { border: 1px solid #c0c0c0; background-color: white; } "
                               "QTabBar::tab { background-color: #f0f0f0; color : black; padding: 10px 20px; margin-right: 2px; } "
                               "QTabBar::tab:selected { background-color: white; color : #6750a4; border-bottom: 2px solid #6750a4; }");

    setupLiveVideoTab();
    setupCapturedImageTab();

    connect(m_tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);

    contentLayout->addWidget(m_tabWidget, 3);

    // 사이드바
    QWidget *sidebarWidget = new QWidget();
    sidebarWidget->setMinimumWidth(250);
    sidebarWidget->setMaximumWidth(350);
    sidebarWidget->setStyleSheet("background-color: #292d41;");

    QVBoxLayout *sidebarLayout = new QVBoxLayout(sidebarWidget);

    m_modeComboBox = new QComboBox();
    m_modeComboBox->addItem("🤖 Automatic");
    m_modeComboBox->addItem("⚙️ Manual");
    m_modeComboBox->setStyleSheet("QComboBox { background-color: #6750a4; color: white; padding: 10px; border: none; border-radius: 5px; font-weight: bold; } QComboBox::drop-down { border: none; } QComboBox::down-arrow { image: none; }");
    sidebarLayout->addWidget(m_modeComboBox);

    setupWarningButtons();
    for (QPushButton *button : m_warningButtons) {
        sidebarLayout->addWidget(button);
    }

    sidebarLayout->addStretch();

    contentLayout->addWidget(sidebarWidget, 1);

    mainLayout->addLayout(contentLayout);
}

void MainWindow::setupLiveVideoTab()
{
    m_liveVideoTab = new QWidget();

    QVBoxLayout *layout = new QVBoxLayout(m_liveVideoTab);

    m_videoStreamWidget = new VideoStreamWidget();
    m_videoStreamWidget->setMinimumHeight(400);
    connect(m_videoStreamWidget, &VideoStreamWidget::clicked, this, &MainWindow::onVideoStreamClicked);
    connect(m_videoStreamWidget, &VideoStreamWidget::streamError, this, &MainWindow::onStreamError);
    layout->setSpacing(10);
    layout->setContentsMargins(5, 5, 5, 5);
    layout->addWidget(m_videoStreamWidget);

    m_streamingButton = new QPushButton("▶️ Start Streaming");
    m_streamingButton->setStyleSheet("QPushButton { background-color: #409cff; color: white; padding: 10px 20px; border: none; border-radius: 5px; font-weight: bold; } QPushButton:hover { background-color: #357abd; } QPushButton:disabled { background-color: #cccccc; }");
    connect(m_streamingButton, &QPushButton::clicked, this, &MainWindow::onStreamingButtonClicked);
    layout->addWidget(m_streamingButton);

    m_tabWidget->addTab(m_liveVideoTab, "📹 Live Video Stream");
}

void MainWindow::setupCapturedImageTab()
{
    m_capturedImageTab = new QWidget();

    QVBoxLayout *mainLayout = new QVBoxLayout(m_capturedImageTab);

    // 컨트롤 영역
    QWidget *controlWidget = new QWidget();
    controlWidget->setStyleSheet("background-color: #f5f5f5; padding: 15px; border-radius: 8px; margin-bottom: 10px;");
    QHBoxLayout *controlLayout = new QHBoxLayout(controlWidget);

    // 날짜 선택
    QLabel *dateLabel = new QLabel("날짜:");
    dateLabel->setStyleSheet("color: black; font-size: 14px; font-weight: bold;");
    controlLayout->addWidget(dateLabel);

    m_dateButton = new QPushButton();
    m_dateButton->setText(m_selectedDate.toString("yyyy-MM-dd (dddd)"));
    m_dateButton->setStyleSheet(
        "QPushButton { "
        "background-color: white; "
        "color: black; "
        "padding: 8px 12px; "
        "border: 1px solid #ccc; "
        "border-radius: 4px; "
        "text-align: left; "
        "min-width: 150px; "
        "} "
        "QPushButton:hover { "
        "background-color: #f0f0f0; "
        "} "
        "QPushButton:pressed { "
        "background-color: #e0e0e0; "
        "}"
        );
    connect(m_dateButton, &QPushButton::clicked, this, &MainWindow::onDateButtonClicked);
    controlLayout->addWidget(m_dateButton);

    // 달력 다이얼로그 설정
    m_calendarDialog = new QDialog(this);
    m_calendarDialog->setWindowTitle("날짜 선택");
    m_calendarDialog->setModal(true);
    m_calendarDialog->setFixedSize(380, 350);
    m_calendarDialog->setStyleSheet(
        "QDialog { "
        "background-color: white; "
        "border-radius: 12px; "
        "}"
        );

    QVBoxLayout *calendarLayout = new QVBoxLayout(m_calendarDialog);

    m_calendarWidget = new QCalendarWidget();
    m_calendarWidget->setSelectedDate(m_selectedDate);
    m_calendarWidget->setStyleSheet(
        "QCalendarWidget { background-color: #ffffff; border: 1px solid #e0e0e0; border-radius: 8px; } "
        "QCalendarWidget QWidget#qt_calendar_navigationbar { background-color: #6750a4; border-top-left-radius: 8px; border-top-right-radius: 8px; } "
        "QCalendarWidget QToolButton { color: white; background-color: transparent; border: none; border-radius: 4px; padding: 8px; font-weight: bold; font-size: 14px; } "
        "QCalendarWidget QToolButton:hover { background-color: rgba(255, 255, 255, 0.2); } "
        "QCalendarWidget QToolButton:pressed { background-color: rgba(255, 255, 255, 0.3); } "
        "QCalendarWidget QMenu { background-color: white; color: black; border: 1px solid #e0e0e0; border-radius: 4px; } "
        "QCalendarWidget QSpinBox { background-color: white; color: #6750a4; border: 1px solid #e0e0e0; border-radius: 4px; padding: 4px; font-weight: bold; selection-background-color: #6750a4; selection-color: white; } "
        "QCalendarWidget QAbstractItemView { background-color: white; color: #333333; selection-background-color: #6750a4; selection-color: white; border: none; outline: none; } "
        "QCalendarWidget QAbstractItemView:enabled { color: #333333; background-color: white; } "
        "QCalendarWidget QAbstractItemView:disabled { color: #cccccc; } "
        "QCalendarWidget QTableView { gridline-color: #f0f0f0; } "
        "QCalendarWidget QHeaderView::section { background-color: #f8f9fa; color: #6750a4; font-weight: bold; border: none; padding: 8px; } "
        "QCalendarWidget QTableView::item { padding: 8px; border-radius: 4px; margin: 1px; } "
        "QCalendarWidget QTableView::item:hover { background-color: #e8f4fd; color: #6750a4; } "
        "QCalendarWidget QTableView::item:selected { background-color: #6750a4; color: white; font-weight: bold; } "
        "QCalendarWidget QTableView::item:selected:hover { background-color: #5a47a0; }"
        );
    connect(m_calendarWidget, &QCalendarWidget::clicked, this, &MainWindow::onCalendarDateSelected);
    calendarLayout->addWidget(m_calendarWidget);

    // 달력 다이얼로그 버튼
    QHBoxLayout *calendarButtonLayout = new QHBoxLayout();
    QPushButton *todayButton = new QPushButton("오늘");
    todayButton->setStyleSheet("QPushButton { background-color: #6750a4; color: white; padding: 10px 20px; border: none; border-radius: 6px; } "
                               "QPushButton:hover { background-color: #5a47a0; }"
                               "QPushButton:pressed { background-color: #4a3d8a;}"
                               );
    connect(todayButton, &QPushButton::clicked, [this]() {
        m_calendarWidget->setSelectedDate(QDate::currentDate());
        onCalendarDateSelected(QDate::currentDate());
    });

    QPushButton *cancelButton = new QPushButton("취소");
    cancelButton->setStyleSheet("QPushButton { background-color: #e0e0e0; color: #333333; padding: 10px 20px; border: none; border-radius: 6px; } "
                                "QPushButton:hover { background-color: #d0d0d0; }"
                                "QPushButton:pressed { background-color: #c0c0c0; }"
                                );
    connect(cancelButton, &QPushButton::clicked, m_calendarDialog, &QDialog::reject);

    calendarButtonLayout->addWidget(todayButton);
    calendarButtonLayout->addStretch();
    calendarButtonLayout->addWidget(cancelButton);
    calendarLayout->addLayout(calendarButtonLayout);

    controlLayout->addSpacing(20);

    // 시간대 선택
    QLabel *hourLabel = new QLabel("시간대:");
    hourLabel->setStyleSheet("color: black; font-size: 14px; font-weight: bold;");
    controlLayout->addWidget(hourLabel);

    m_hourComboBox = new QComboBox();
    m_hourComboBox->setStyleSheet(
        "QComboBox { background-color: white; color: black; padding: 8px; border: 1px solid #ccc; border-radius: 4px; min-width: 100px; } "
        "QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: center right; width: 20px; border-left-width: 1px; border-left-color: #ccc; border-left-style: solid; border-top-right-radius: 3px; border-bottom-right-radius: 3px; } "
        "QComboBox::down-arrow { image: none; border-left: 5px solid transparent; border-right: 5px solid transparent; border-top: 5px solid #666; } "
        "QComboBox QAbstractItemView { background-color: white; color: black; selection-background-color: #e3f2fd; border: 1px solid #ccc; }"
        );

    // 24시간 시간대 추가
    for (int hour = 0; hour < 24; hour++) {
        QString timeRange = QString("%1시 ~ %2시").arg(hour).arg(hour + 1);
        m_hourComboBox->addItem(timeRange, hour);
    }

    // 현재 시간으로 기본값 설정
    m_hourComboBox->setCurrentIndex(QTime::currentTime().hour());

    connect(m_hourComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onHourComboChanged);
    controlLayout->addWidget(m_hourComboBox);

    controlLayout->addSpacing(20);

    m_requestButton = new QPushButton("📷 이미지 요청");
    m_requestButton->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; padding: 10px 20px; border: none; border-radius: 5px; font-weight: bold; } QPushButton:hover { background-color: #45a049; } QPushButton:disabled { background-color: #cccccc; }");
    connect(m_requestButton, &QPushButton::clicked, this, &MainWindow::onRequestImagesClicked);
    controlLayout->addWidget(m_requestButton);

    controlLayout->addStretch();

    m_statusLabel = new QLabel("서버에 연결하여 이미지를 요청하세요.");
    m_statusLabel->setStyleSheet("color: #666; font-size: 12px;");
    controlLayout->addWidget(m_statusLabel);

    mainLayout->addWidget(controlWidget);

    // 이미지 영역
    m_imageScrollArea = new QScrollArea();
    m_imageScrollArea->setWidgetResizable(true);
    m_imageScrollArea->setStyleSheet("QScrollArea { border: 1px solid #ddd; background-color: white; }");

    m_imageGridWidget = new QWidget();
    m_imageGridWidget->setStyleSheet("background-color: white;");
    m_imageGridLayout = new QGridLayout(m_imageGridWidget);
    m_imageGridLayout->setSpacing(15);
    m_imageGridLayout->setContentsMargins(15, 15, 15, 15);

    QLabel *emptyLabel = new QLabel("이미지 요청 버튼을 눌러 해당 시간대의 이미지를 불러오세요.");
    emptyLabel->setAlignment(Qt::AlignCenter);
    emptyLabel->setStyleSheet("color: #999; font-size: 16px; padding: 50px;");
    m_imageGridLayout->addWidget(emptyLabel, 0, 0, 1, 2);

    m_imageScrollArea->setWidget(m_imageGridWidget);
    mainLayout->addWidget(m_imageScrollArea);

    m_tabWidget->addTab(m_capturedImageTab, "📷 Captured Images");
}

void MainWindow::setupWarningButtons()
{
    for (int i = 0; i < 4; ++i) {
        QPushButton *button = new QPushButton(QString("Warning %1 (OFF)").arg(i + 1));
        button->setFixedHeight(50);
        button->setProperty("warningIndex", i);
        connect(button, &QPushButton::clicked, this, &MainWindow::onWarningButtonClicked);
        m_warningButtons.append(button);
    }

    updateWarningButtonStyles();
}

void MainWindow::setupNetworkConnection()
{
    m_networkManager = new QNetworkAccessManager(this);

    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, &MainWindow::updateLogDisplay);
    m_updateTimer->start(5000);

    m_tcpCommunicator = new TcpCommunicator(this);

    connect(m_tcpCommunicator, &TcpCommunicator::connected, this, &MainWindow::onTcpConnected);
    connect(m_tcpCommunicator, &TcpCommunicator::disconnected, this, &MainWindow::onTcpDisconnected);
    connect(m_tcpCommunicator, &TcpCommunicator::errorOccurred, this, &MainWindow::onTcpError);
    connect(m_tcpCommunicator, &TcpCommunicator::dataReceived, this, &MainWindow::onTcpDataReceived);
    connect(m_tcpCommunicator, &TcpCommunicator::packetReceived, this, &MainWindow::onTcpPacketReceived);
    connect(m_tcpCommunicator, &TcpCommunicator::imagesReceived, this, &MainWindow::onImagesReceived);

    // 이미지 요청 타임아웃 타이머 설정
    m_requestTimeoutTimer = new QTimer(this);
    m_requestTimeoutTimer->setSingleShot(true);
    m_requestTimeoutTimer->setInterval(30000);
    connect(m_requestTimeoutTimer, &QTimer::timeout, this, &MainWindow::onRequestTimeout);

    m_imageViewerDialog = new ImageViewerDialog(this);
}

void MainWindow::applyStyles()
{
    setStyleSheet("QMainWindow { background-color: #292d41; }");
}

QString MainWindow::getWarningButtonStyle(bool isActive)
{
    if (isActive) {
        return "QPushButton { background-color: #cf5e2d; color: white; padding: 15px; border: none; border-radius: 5px; font-weight: bold; } QPushButton:hover { background-color: #b54d26; }";
    } else {
        return "QPushButton { background-color: #b4afb9; color: white; padding: 15px; border: none; border-radius: 5px; font-weight: bold; } QPushButton:hover { background-color: #a09ca7; }";
    }
}

void MainWindow::updateWarningButtonStyles()
{
    for (int i = 0; i < m_warningButtons.size(); ++i) {
        bool isActive = m_warningStates[i];
        m_warningButtons[i]->setStyleSheet(getWarningButtonStyle(isActive));
        m_warningButtons[i]->setText(QString("Warning %1 (%2)").arg(i + 1).arg(isActive ? "ON" : "OFF"));
    }
}

void MainWindow::clearImageGrid()
{
    QLayoutItem *item;
    while ((item = m_imageGridLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
}

void MainWindow::displayImages(const QList<ImageData> &images)
{
    clearImageGrid();

    if (images.isEmpty()) {
        QLabel *emptyLabel = new QLabel("해당 시간대에 캡처된 이미지가 없습니다.");
        emptyLabel->setAlignment(Qt::AlignCenter);
        emptyLabel->setStyleSheet("color: #999; font-size: 16px; padding: 50px;");
        m_imageGridLayout->addWidget(emptyLabel, 0, 0, 1, 2);
        return;
    }

    int row = 0;
    int col = 0;

    for (const ImageData &imageData : images) {
        ClickableImageLabel *imageLabel = new ClickableImageLabel();
        imageLabel->setFixedSize(300, 200);
        imageLabel->setScaledContents(true);
        imageLabel->setImageData(imageData.imagePath, imageData.timestamp, imageData.logText);

        QPixmap pixmap;
        if (pixmap.load(imageData.imagePath)) {
            imageLabel->setPixmap(pixmap);
        } else {
            imageLabel->setText("이미지 로드 실패");
            imageLabel->setStyleSheet(imageLabel->styleSheet() + " color: #999;");
        }

        QLabel *timeLabel = new QLabel(imageData.timestamp);
        timeLabel->setAlignment(Qt::AlignCenter);
        timeLabel->setStyleSheet("background-color: rgba(0,0,0,0.7); color: white; padding: 5px; border-radius: 3px; font-size: 12px;");

        QWidget *container = new QWidget();
        container->setFixedSize(320, 240);
        QVBoxLayout *containerLayout = new QVBoxLayout(container);
        containerLayout->setContentsMargins(0, 0, 0, 0);
        containerLayout->addWidget(imageLabel);
        containerLayout->addWidget(timeLabel);

        connect(imageLabel, &ClickableImageLabel::clicked, this, &MainWindow::onImageClicked);

        m_imageGridLayout->addWidget(container, row, col);

        col++;
        if (col >= 2) {
            col = 0;
            row++;
        }
    }

    m_imageGridWidget->adjustSize();
}

void MainWindow::onTabChanged(int index)
{
    if (index == 1) {
        m_modeComboBox->setVisible(false);
        for (QPushButton *button : m_warningButtons) {
            button->setVisible(false);
        }
    } else {
        m_modeComboBox->setVisible(true);
        for (QPushButton *button : m_warningButtons) {
            button->setVisible(true);
        }
    }
}

void MainWindow::onWarningButtonClicked()
{
    QPushButton *button = qobject_cast<QPushButton*>(sender());
    if (!button) return;

    int index = button->property("warningIndex").toInt();

    m_warningStates[index] = !m_warningStates[index];

    updateWarningButtonStyles();

    qDebug() << QString("Warning %1 상태 변경: %2").arg(index + 1).arg(m_warningStates[index] ? "ON" : "OFF");
}

void MainWindow::onNetworkConfigClicked()
{
    if (!m_networkDialog) {
        m_networkDialog = new NetworkConfigDialog(this);
        m_networkDialog->setRtspUrl(m_rtspUrl);
        m_networkDialog->setTcpHost(m_tcpHost);
        m_networkDialog->setTcpPort(m_tcpPort);
    }

    if (m_networkDialog->exec() == QDialog::Accepted) {
        m_rtspUrl = m_networkDialog->getRtspUrl();
        m_tcpHost = m_networkDialog->getTcpHost();
        m_tcpPort = m_networkDialog->getTcpPort();

        if (m_videoStreamWidget) {
            m_videoStreamWidget->setStreamUrl(m_rtspUrl);
        }

        if (m_tcpCommunicator) {
            m_tcpCommunicator->connectToServer(m_tcpHost, m_tcpPort);
        }

        qDebug() << "네트워크 설정 업데이트:" << m_rtspUrl << m_tcpHost << m_tcpPort;
    }
}

void MainWindow::onVideoStreamClicked()
{
    if (!m_videoStreamWidget->isStreaming()) {
        QMessageBox::information(this, "안내", "먼저 스트리밍을 시작해주세요.");
        return;
    }

    if (!m_lineDrawingDialog) {
        m_lineDrawingDialog = new LineDrawingDialog(this);
        connect(m_lineDrawingDialog, &LineDrawingDialog::lineDrawn, this, &MainWindow::sendLineCoordinates);
    }

    m_lineDrawingDialog->setStreamUrl(m_rtspUrl);
    m_lineDrawingDialog->exec();
}

void MainWindow::onDateChanged(const QDate &date)
{
    qDebug() << "날짜 변경:" << date.toString("yyyy-MM-dd");
    m_statusLabel->setText(QString("선택된 날짜: %1").arg(date.toString("yyyy-MM-dd")));
}

void MainWindow::onHourChanged(int hour)
{
    if (m_hourSpinBox) {
        m_hourSpinBox->setSuffix(QString("시 ~ %1시").arg(hour + 1));
    }
    qDebug() << "시간 변경:" << QString("%1시~%2시").arg(hour).arg(hour + 1);
}

void MainWindow::onDateButtonClicked()
{
    if (m_calendarDialog) {
        m_calendarWidget->setSelectedDate(m_selectedDate);
        m_calendarDialog->exec();
    }
}

void MainWindow::onCalendarDateSelected(const QDate &date)
{
    m_selectedDate = date;
    m_dateButton->setText(date.toString("yyyy-MM-dd (dddd)"));
    m_calendarDialog->accept();

    qDebug() << "달력에서 날짜 선택:" << date.toString("yyyy-MM-dd");
    m_statusLabel->setText(QString("선택된 날짜: %1").arg(date.toString("yyyy-MM-dd")));
}

void MainWindow::onHourComboChanged(int index)
{
    int selectedHour = m_hourComboBox->itemData(index).toInt();
    qDebug() << "시간 변경:" << QString("%1시~%2시").arg(selectedHour).arg(selectedHour + 1);
}

void MainWindow::onStreamingButtonClicked()
{
    if (m_videoStreamWidget) {
        if (m_videoStreamWidget->isStreaming()) {
            m_videoStreamWidget->stopStream();
            m_streamingButton->setText("▶️ Start Streaming");
            m_streamingButton->setStyleSheet("QPushButton { background-color: #409cff; color: white; padding: 10px 20px; border: none; border-radius: 5px; font-weight: bold; } QPushButton:hover { background-color: #357abd; }");
        } else {
            if (m_rtspUrl.isEmpty()) {
                QMessageBox::warning(this, "설정 오류", "먼저 네트워크 설정에서 RTSP URL을 설정해주세요.");
                return;
            }

            m_videoStreamWidget->startStream(m_rtspUrl);
            m_streamingButton->setText("⏸️ Stop Streaming");
            m_streamingButton->setStyleSheet("QPushButton { background-color: #dc3545; color: white; padding: 10px 20px; border: none; border-radius: 5px; font-weight: bold; } QPushButton:hover { background-color: #c82333; }");
        }
    }
}

void MainWindow::onRequestImagesClicked()
{
    if (!m_tcpCommunicator || !m_tcpCommunicator->isConnected()) {
        QMessageBox::warning(this, "연결 오류", "서버에 연결되어 있지 않습니다.");
        return;
    }

    int selectedHour = m_hourComboBox->currentData().toInt();
    QString dateString = m_selectedDate.toString("yyyy-MM-dd");

    m_statusLabel->setText("이미지 요청 중... (30초 후 타임아웃)");
    m_requestButton->setEnabled(false);

    m_requestTimeoutTimer->start();

    m_tcpCommunicator->requestImagesByDateAndHour(dateString, selectedHour);

    qDebug() << QString("이미지 요청: %1, %2시~%3시").arg(dateString).arg(selectedHour).arg(selectedHour + 1);
}

void MainWindow::sendLineCoordinates(const QPoint &start, const QPoint &end)
{
    if (m_tcpCommunicator && m_tcpCommunicator->isConnected()) {
        m_tcpCommunicator->sendLineCoordinates(start, end);
        qDebug() << "기준선 좌표 전송 성공:" << start << "to" << end;
    } else {
        qDebug() << "TCP 연결이 없어 좌표 전송 실패";
    }
}

void MainWindow::onTcpConnected()
{
    m_isConnected = true;
    qDebug() << "TCP 서버 연결 성공";

    m_statusLabel->setText("TCP 서버 연결 성공!");
    QMessageBox::information(this, "연결 성공", "TCP 서버에 성공적으로 연결되었습니다.");

    if (m_requestButton) {
        m_requestButton->setEnabled(true);
        m_statusLabel->setText("서버 연결됨. 이미지를 요청할 수 있습니다.");
    }
}

void MainWindow::onTcpDisconnected()
{
    m_isConnected = false;
    qDebug() << "TCP 서버 연결 해제";

    if (m_requestButton) {
        m_requestButton->setEnabled(false);
        m_statusLabel->setText("서버 연결이 끊어졌습니다.");
    }
}

void MainWindow::onTcpError(const QString &error)
{
    qDebug() << "TCP 에러:" << error;
    QMessageBox::warning(this, "TCP 연결 오류", error);
}

void MainWindow::onTcpDataReceived(const QString &data)
{
    qDebug() << "TCP 데이터 수신:" << data;
}

void MainWindow::onTcpPacketReceived(int requestId, int success, const QString &/*data1*/, const QString &/*data2*/, const QString &/*data3*/)
{
    qDebug() << QString("TCP 패킷 수신 - ID: %1, 성공: %2").arg(requestId).arg(success);
}

void MainWindow::onImagesReceived(const QList<ImageData> &images)
{
    qDebug() << QString("이미지 리스트 수신: %1개").arg(images.size());

    if (m_requestTimeoutTimer->isActive()) {
        m_requestTimeoutTimer->stop();
    }

    displayImages(images);

    m_statusLabel->setText(QString("이미지 %1개를 불러왔습니다.").arg(images.size()));
    m_requestButton->setEnabled(true);
}

void MainWindow::onImageClicked(const QString &imagePath, const QString &timestamp, const QString &logText)
{
    QPixmap pixmap;
    if (pixmap.load(imagePath)) {
        m_imageViewerDialog->setImage(pixmap, timestamp, logText);
        m_imageViewerDialog->exec();
    } else {
        QMessageBox::warning(this, "이미지 로드 오류", "이미지를 불러올 수 없습니다.");
    }
}

void MainWindow::updateLogDisplay()
{
    // 주기적으로 로그 업데이트 (필요시 구현)
}

void MainWindow::onRequestTimeout()
{
    qDebug() << "이미지 요청 타임아웃 (30초)";

    m_statusLabel->setText("이미지 요청 타임아웃. 서버 응답이 없습니다. 다시 시도해주세요.");
    m_requestButton->setEnabled(true);

    QMessageBox::warning(this, "요청 타임아웃",
                         "서버에서 30초 내에 응답이 없습니다.\n"
                         "네트워크 연결을 확인하고 다시 시도해주세요.");
}

void MainWindow::onStreamError(const QString &error)
{
    qDebug() << "스트림 에러:" << error;
    QMessageBox::warning(this, "스트림 오류", error);

    // 스트리밍 버튼 상태 복원
    if (m_streamingButton) {
        m_streamingButton->setText("▶️ Start Streaming");
        m_streamingButton->setStyleSheet("QPushButton { background-color: #409cff; color: white; padding: 10px 20px; border: none; border-radius: 5px; font-weight: bold; } QPushButton:hover { background-color: #357abd; }");
    }
}
