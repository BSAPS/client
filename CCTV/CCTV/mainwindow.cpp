#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QApplication>         // 애플리케이션 클래스
#include <QMessageBox>          // 메시지 박스
#include <QSplitter>            // 스플리터 위젯
#include <QTimer>               // 타이머
#include <QDateTime>            // 날짜/시간

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_networkClient(nullptr)
    , m_videoStreamer(nullptr)
    , m_imageViewer(nullptr)
    , m_isConnected(false)
    , m_isStreaming(false)
{
    ui->setupUi(this);                      // UI 파일에서 정의된 UI 설정
    
    // LCD 상태 초기화
    for (int i = 1; i <= 4; ++i) {
        m_lcdStates[i] = false;
    }
    
    // 컴포넌트 초기화
    m_networkClient = new NetworkClient(this);      // 네트워크 클라이언트 생성
    m_videoStreamer = new VideoStreamer(this);      // 비디오 스트리머 생성
    m_imageViewer = new ImageViewer(this);          // 이미지 뷰어 생성
    
    // UI 추가 설정
    setupUIComponents();
    
    // 네트워크 클라이언트 시그널 연결
    connect(m_networkClient, &NetworkClient::connected, this, &MainWindow::onNetworkConnected);
    connect(m_networkClient, &NetworkClient::disconnected, this, &MainWindow::onNetworkDisconnected);
    connect(m_networkClient, &NetworkClient::connectionError, this, &MainWindow::onNetworkError);
    connect(m_networkClient, &NetworkClient::videoFrameReceived, this, &MainWindow::onVideoFrameReceived);
    connect(m_networkClient, &NetworkClient::capturedImageReceived, this, &MainWindow::onCapturedImageReceived);
    connect(m_networkClient, &NetworkClient::statusReceived, this, &MainWindow::onStatusReceived);
    
    // 비디오 스트리머 시그널 연결
    connect(m_videoStreamer, &VideoStreamer::streamingStarted, this, &MainWindow::onVideoStreamingStarted);
    connect(m_videoStreamer, &VideoStreamer::streamingStopped, this, &MainWindow::onVideoStreamingStopped);
    
    // 이미지 뷰어 시그널 연결
    connect(m_imageViewer, &ImageViewer::imageAdded, this, &MainWindow::onImageAdded);
    
    // UI 버튼 시그널 연결
    connect(ui->connectButton, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(ui->disconnectButton, &QPushButton::clicked, this, &MainWindow::onDisconnectClicked);
    connect(ui->startStreamButton, &QPushButton::clicked, this, &MainWindow::onStartStreamClicked);
    connect(ui->stopStreamButton, &QPushButton::clicked, this, &MainWindow::onStopStreamClicked);
    connect(ui->captureButton, &QPushButton::clicked, this, &MainWindow::onCaptureImageClicked);
    connect(ui->lcd1Button, &QPushButton::clicked, this, &MainWindow::onLCD1Clicked);
    connect(ui->lcd2Button, &QPushButton::clicked, this, &MainWindow::onLCD2Clicked);
    connect(ui->lcd3Button, &QPushButton::clicked, this, &MainWindow::onLCD3Clicked);
    connect(ui->lcd4Button, &QPushButton::clicked, this, &MainWindow::onLCD4Clicked);
    
    // 메뉴 액션 연결
    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::showAbout);
    connect(ui->actionFullscreen, &QAction::triggered, this, &MainWindow::toggleFullscreen);
    
    // 날짜/시간 업데이트 타이머 설정
    QTimer *dateTimeTimer = new QTimer(this);
    connect(dateTimeTimer, &QTimer::timeout, this, &MainWindow::updateDateTime);
    dateTimeTimer->start(1000);             // 1초마다 업데이트
    updateDateTime();                       // 초기 날짜/시간 설정
    
    logMessage("🚀 CCTV Client initialized successfully");
    logMessage("💡 Ready to connect to Raspberry Pi server");
}

MainWindow::~MainWindow()
{
    if (m_networkClient && m_networkClient->isConnected()) {
        m_networkClient->disconnectFromServer();    // 서버 연결 해제
    }
    delete ui;                              // UI 객체 메모리 해제
}

void MainWindow::setupUIComponents()
{
    // 비디오 스트리머에 라벨 설정
    m_videoStreamer->setDisplayLabel(ui->videoLabel);
    
    // 이미지 뷰어를 이미지 탭에 추가
    QVBoxLayout *imageLayout = qobject_cast<QVBoxLayout*>(ui->imageTab->layout());
    if (imageLayout) {
        // 플레이스홀더 제거
        QLayoutItem *item = imageLayout->takeAt(0);
        if (item) {
            delete item->widget();
            delete item;
        }
        // 이미지 뷰어 추가
        imageLayout->addWidget(m_imageViewer);
    }
    
    // 상태바 설정
    ui->statusbar->addWidget(ui->statusBarLabel);
    ui->statusBarLabel->setText("🔴 Disconnected");
    
    // 초기 상태 설정
    updateConnectionStatus(false);
}

void MainWindow::onConnectClicked()
{
    QString host = ui->hostLineEdit->text().trimmed();    // 서버 주소 가져오기
    quint16 port = ui->portSpinBox->value();              // 포트 번호 가져오기
    
    if (host.isEmpty()) {
        QMessageBox::warning(this, "⚠️ Connection Error", "Please enter server address");
        return;
    }
    
    logMessage(QString("🔄 Connecting to %1:%2...").arg(host).arg(port));
    m_networkClient->connectToServer(host, port);       // 서버 연결 시도
    
    ui->connectButton->setEnabled(false);                 // 연결 버튼 비활성화
    ui->connectionStatusLabel->setText("Status: Connecting...");
    ui->connectionStatusLabel->setStyleSheet(
        "QLabel { color: #ff8c00; font-weight: bold; font-size: 13px; "
        "padding: 8px; background-color: rgba(255, 140, 0, 0.1); border-radius: 4px; }"
    );
}

void MainWindow::onDisconnectClicked()
{
    logMessage("🔌 Disconnecting from server...");
    m_networkClient->disconnectFromServer();            // 서버 연결 해제
}

void MainWindow::onNetworkConnected()
{
    m_isConnected = true;
    updateConnectionStatus(true);
    logMessage("✅ Successfully connected to server");
}

void MainWindow::onNetworkDisconnected()
{
    m_isConnected = false;
    updateConnectionStatus(false);
    
    // 스트리밍 정지
    if (m_isStreaming) {
        m_videoStreamer->stopStreaming();
    }
    
    logMessage("❌ Disconnected from server");
}

void MainWindow::onNetworkError(const QString &error)
{
    ui->connectButton->setEnabled(true);
    ui->connectionStatusLabel->setText("Status: Connection Failed");
    ui->connectionStatusLabel->setStyleSheet(
        "QLabel { color: #d13438; font-weight: bold; font-size: 13px; "
        "padding: 8px; background-color: rgba(209, 52, 56, 0.1); border-radius: 4px; }"
    );
    
    logMessage(QString("❌ Network error: %1").arg(error));
    QMessageBox::critical(this, "🚫 Network Error", error);
}

void MainWindow::updateConnectionStatus(bool connected)
{
    if (connected) {
        ui->connectButton->setEnabled(false);
        ui->disconnectButton->setEnabled(true);
        ui->connectionStatusLabel->setText("Status: Connected");
        ui->connectionStatusLabel->setStyleSheet(
            "QLabel { color: #107c10; font-weight: bold; font-size: 13px; "
            "padding: 8px; background-color: rgba(16, 124, 16, 0.1); border-radius: 4px; }"
        );
        ui->connectionStatusIcon->setText("🟢");
        ui->statusBarLabel->setText("🟢 Connected");
    } else {
        ui->connectButton->setEnabled(true);
        ui->disconnectButton->setEnabled(false);
        ui->connectionStatusLabel->setText("Status: Disconnected");
        ui->connectionStatusLabel->setStyleSheet(
            "QLabel { color: #d13438; font-weight: bold; font-size: 13px; "
            "padding: 8px; background-color: rgba(209, 52, 56, 0.1); border-radius: 4px; }"
        );
        ui->connectionStatusIcon->setText("🔴");
        ui->statusBarLabel->setText("🔴 Disconnected");
    }
}

void MainWindow::onVideoFrameReceived(const QByteArray &frameData)
{
    if (m_isStreaming) {
        m_videoStreamer->addFrame(frameData);            // 비디오 스트리머에 프레임 추가
        
        // FPS 업데이트
        double fps = m_videoStreamer->getCurrentFPS();
        ui->fpsLabel->setText(QString("FPS: %1").arg(fps, 0, 'f', 1));
    }
}

void MainWindow::onCapturedImageReceived(const QByteArray &imageData)
{
    m_imageViewer->addImage(imageData);                  // 이미지 뷰어에 이미지 추가
    logMessage("📸 Captured image received from server");
    
    // 이미지 탭으로 자동 전환
    ui->mainTabWidget->setCurrentWidget(ui->imageTab);
}

void MainWindow::onStatusReceived(const QString &status)
{
    logMessage(QString("📊 Server status: %1").arg(status));
}

void MainWindow::onStartStreamClicked()
{
    if (!m_isConnected) {
        QMessageBox::warning(this, "⚠️ Not Connected", "Please connect to server first");
        return;
    }
    
    m_networkClient->startVideoStream();                // 서버에 스트리밍 시작 요청
    m_videoStreamer->startStreaming();                   // 로컬 스트리머 시작
    
    logMessage("📹 Video streaming start requested");
}

void MainWindow::onStopStreamClicked()
{
    if (m_isConnected) {
        m_networkClient->stopVideoStream();             // 서버에 스트리밍 정지 요청
    }
    m_videoStreamer->stopStreaming();                    // 로컬 스트리머 정지
    
    logMessage("⏹️ Video streaming stopped");
}

void MainWindow::onCaptureImageClicked()
{
    if (!m_isConnected) {
        QMessageBox::warning(this, "⚠️ Not Connected", "Please connect to server first");
        return;
    }
    
    m_networkClient->requestImageCapture();              // 서버에 이미지 캡처 요청
    logMessage("📷 Image capture requested from server");
}

void MainWindow::onLCD1Clicked()
{
    controlLCD(1);
}

void MainWindow::onLCD2Clicked()
{
    controlLCD(2);
}

void MainWindow::onLCD3Clicked()
{
    controlLCD(3);
}

void MainWindow::onLCD4Clicked()
{
    controlLCD(4);
}

void MainWindow::controlLCD(int lcdId)
{
    if (!m_isConnected) {
        QMessageBox::warning(this, "⚠️ Not Connected", "Please connect to server first");
        return;
    }
    
    bool newState = !m_lcdStates[lcdId];                     // 상태 토글
    m_lcdStates[lcdId] = newState;
    m_networkClient->controlLCD(lcdId, newState);            // 서버에 LCD 제어 명령 전송
    
    QString stateText = newState ? "ON" : "OFF";
    QString buttonText = QString("💡 LCD %1 (%2)").arg(lcdId).arg(stateText);
    
    // 해당 버튼 텍스트 업데이트
    QPushButton *button = nullptr;
    switch (lcdId) {
    case 1: button = ui->lcd1Button; break;
    case 2: button = ui->lcd2Button; break;
    case 3: button = ui->lcd3Button; break;
    case 4: button = ui->lcd4Button; break;
    }
    
    if (button) {
        button->setText(buttonText);
        // 상태에 따른 색상 변경
        QString color = newState ? "#32cd32" : "#ff6b35";
        button->setStyleSheet(QString(
            "QPushButton { background-color: %1; min-height: 50px; "
            "font-size: 13px; font-weight: bold; border-radius: 8px; }"
            "QPushButton:hover { background-color: %2; }"
        ).arg(color).arg(newState ? "#28a428" : "#e55a2b"));
    }
    
    logMessage(QString("💡 LCD %1 turned %2").arg(lcdId).arg(stateText));
}

void MainWindow::updateDateTime()
{
    QDateTime currentDateTime = QDateTime::currentDateTime();
    ui->dateTimeLabel->setText(currentDateTime.toString("yyyy-MM-dd hh:mm:ss"));
}

void MainWindow::onVideoStreamingStarted()
{
    m_isStreaming = true;
    ui->startStreamButton->setEnabled(false);
    ui->stopStreamButton->setEnabled(true);
    ui->videoStatusLabel->setText("Status: Streaming");
    ui->videoStatusLabel->setStyleSheet(
        "QLabel { color: #107c10; font-weight: bold; padding: 8px; "
        "background-color: rgba(16, 124, 16, 0.1); border-radius: 4px; }"
    );
}

void MainWindow::onVideoStreamingStopped()
{
    m_isStreaming = false;
    ui->startStreamButton->setEnabled(true);
    ui->stopStreamButton->setEnabled(false);
    ui->videoStatusLabel->setText("Status: Stopped");
    ui->videoStatusLabel->setStyleSheet(
        "QLabel { color: #d13438; font-weight: bold; padding: 8px; "
        "background-color: rgba(209, 52, 56, 0.1); border-radius: 4px; }"
    );
    ui->fpsLabel->setText("FPS: 0");
    
    // 비디오 라벨 클리어
    ui->videoLabel->clear();
    ui->videoLabel->setText("📺 No Video Stream\nClick 'Start Streaming' to begin");
}

void MainWindow::onImageAdded(int totalCount)
{
    logMessage(QString("🖼️ New image added. Total images: %1").arg(totalCount));
}

void MainWindow::showAbout()
{
    QMessageBox::about(this, "About CCTV Client",
        "🎥 <b>CCTV Remote Client</b><br><br>"
        "Professional monitoring system for Raspberry Pi CCTV<br><br>"
        "Features:<br>"
        "• Real-time video streaming<br>"
        "• Remote image capture<br>"
        "• LCD control panel<br>"
        "• Network monitoring<br><br>"
        "Version 1.0<br>"
        "Built with Qt Framework"
    );
}

void MainWindow::toggleFullscreen()
{
    if (isFullScreen()) {
        showNormal();
    } else {
        showFullScreen();
    }
}

void MainWindow::logMessage(const QString &message)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    QString logEntry = QString("[%1] %2").arg(timestamp, message);
    
    // HTML 형식으로 색상 적용
    QString coloredEntry;
    if (message.contains("✅") || message.contains("🟢")) {
        coloredEntry = QString("<span style='color: #00ff00;'>[%1] %2</span>").arg(timestamp, message);
    } else if (message.contains("❌") || message.contains("🔴")) {
        coloredEntry = QString("<span style='color: #ff4444;'>[%1] %2</span>").arg(timestamp, message);
    } else if (message.contains("⚠️") || message.contains("🟡")) {
        coloredEntry = QString("<span style='color: #ffff00;'>[%1] %2</span>").arg(timestamp, message);
    } else {
        coloredEntry = QString("<span style='color: #ffffff;'>[%1] %2</span>").arg(timestamp, message);
    }
    
    ui->logTextEdit->append(coloredEntry);
    
    // 로그 라인 수 제한 (최대 100줄)
    QStringList lines = ui->logTextEdit->toPlainText().split('\n');
    if (lines.size() > 100) {
        ui->logTextEdit->clear();
        for (int i = lines.size() - 100; i < lines.size(); ++i) {
            ui->logTextEdit->append(lines[i]);
        }
    }
    
    // 로그 영역을 맨 아래로 스크롤
    QTextCursor cursor = ui->logTextEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    ui->logTextEdit->setTextCursor(cursor);
}
