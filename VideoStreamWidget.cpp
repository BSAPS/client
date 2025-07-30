#include "VideoStreamWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QApplication>
#include <QDebug>
#include <QThread>
#include <QUrl>
#include <QMessageBox>
#include <QPushButton>

VideoStreamWidget::VideoStreamWidget(QWidget *parent)
    : QWidget(parent)
    , m_videoWidget(nullptr)
    , m_statusLabel(nullptr)
    , m_liveIndicator(nullptr)
    , m_layout(nullptr)
    , m_mediaPlayer(nullptr)
    , m_audioOutput(nullptr)
    , m_connectionTimer(nullptr)
    , m_liveBlinkTimer(nullptr)
    , m_statusUpdateTimer(nullptr)
    , m_isStreaming(false)
    , m_reconnectAttempts(0)
{
    setupUI();
    setupMediaPlayer();
    setupTimers();
}

VideoStreamWidget::~VideoStreamWidget()
{
    stopStream();
    if (m_mediaPlayer) {
        delete m_mediaPlayer;
    }
    if (m_audioOutput) {
        delete m_audioOutput;
    }
}

void VideoStreamWidget::setupUI()
{
    m_layout = new QVBoxLayout(this);
    m_layout->setSpacing(0);
    m_layout->setContentsMargins(0, 0, 0, 0);

    // 상태 표시 영역 (상단 바)
    QWidget *statusWidget = new QWidget();
    statusWidget->setFixedHeight(50);  // 고정 높이
    QHBoxLayout *statusLayout = new QHBoxLayout(statusWidget);
    statusLayout->setContentsMargins(10, 4, 10, 4);
    statusLayout->setSpacing(10);

    // LIVE Indicator
    m_liveIndicator = new QLabel("●");
    m_liveIndicator->setStyleSheet("color: red; font-size: 14px; font-weight: bold;");
    m_liveIndicator->setVisible(false);
    m_liveIndicator->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    statusLayout->addWidget(m_liveIndicator);

    // 상태 텍스트
    m_statusLabel = new QLabel("스트림 대기 중...");
    m_statusLabel->setStyleSheet("color: #cccccc; font-size: 13px;");
    m_statusLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    statusLayout->addWidget(m_statusLabel);

    statusLayout->addStretch();

    // draw 버튼 추가
    QPushButton *drawButton = new QPushButton();
    drawButton->setIcon(QIcon(":/icons/draw.png"));  // 아이콘 경로 확인
    drawButton->setIconSize(QSize(22, 22));
    drawButton->setFixedSize(36, 36);
    drawButton->setCursor(Qt::PointingHandCursor);
    drawButton->setStyleSheet(
        "QPushButton { background-color: #3b3e52; border: none; border-radius: 6px; }"
        "QPushButton:hover { background-color: #4b4f68; }"
        );
    connect(drawButton, &QPushButton::clicked, this, [=]() {
        emit drawButtonClicked();  // MainWindow 연결 필요
    });
    statusLayout->addWidget(drawButton);

    m_layout->addWidget(statusWidget);
    // 비디오 표시 영역
    m_videoWidget = new QVideoWidget();
    m_videoWidget->setMinimumSize(640, 480);
    m_videoWidget->setStyleSheet("border: 2px solid #ddd; background-color: #000000;");
    m_layout->addWidget(m_videoWidget);

    setLayout(m_layout);
}


void VideoStreamWidget::setupMediaPlayer()
{
    m_mediaPlayer = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    
    m_mediaPlayer->setAudioOutput(m_audioOutput);
    m_mediaPlayer->setVideoOutput(m_videoWidget);
    
    // 미디어 플레이어 시그널 연결
    connect(m_mediaPlayer, &QMediaPlayer::mediaStatusChanged,
            this, &VideoStreamWidget::onMediaStatusChanged);
    connect(m_mediaPlayer, &QMediaPlayer::playbackStateChanged,
            this, &VideoStreamWidget::onPlaybackStateChanged);
    connect(m_mediaPlayer, &QMediaPlayer::errorOccurred,
            this, &VideoStreamWidget::onErrorOccurred);
}

void VideoStreamWidget::setupTimers()
{
    // 연결 타임아웃 타이머
    m_connectionTimer = new QTimer(this);
    m_connectionTimer->setSingleShot(true);
    m_connectionTimer->setInterval(30000);
    connect(m_connectionTimer, &QTimer::timeout, this, &VideoStreamWidget::onConnectionTimeout);
    
    // LIVE 표시 깜빡임 타이머
    m_liveBlinkTimer = new QTimer(this);
    m_liveBlinkTimer->setInterval(1000); // 1초
    connect(m_liveBlinkTimer, &QTimer::timeout, [this]() {
        if (m_liveIndicator->isVisible()) {
            QString currentColor = m_liveIndicator->styleSheet().contains("#4caf50") ? "#4caf50" : "#4caf50";
            m_liveIndicator->setStyleSheet(QString("color: %1; font-size: 16px; font-weight: bold;").arg(currentColor));
        }
    });
    
    // 상태 업데이트 타이머
    m_statusUpdateTimer = new QTimer(this);
    m_statusUpdateTimer->setInterval(3000); // 5초마다
    connect(m_statusUpdateTimer, &QTimer::timeout, this, &VideoStreamWidget::updateConnectionStatus);
}

void VideoStreamWidget::startStream(const QString &rtspUrl)
{
    if (m_isStreaming) {
        stopStream();
    }
    
    m_rtspUrl = rtspUrl;
    m_reconnectAttempts = 0;
    
    qDebug() << "스트림 시작 시도:" << rtspUrl;
    
    showConnectionStatus("연결 중...", "#ff9800");
    m_connectionTimer->start();
    m_statusUpdateTimer->start();
    
    // RTSP URL 설정 및 재생 시작
    QUrl url(rtspUrl);
    m_mediaPlayer->setSource(url);
    m_mediaPlayer->play();
    
    m_isStreaming = true;
}

void VideoStreamWidget::stopStream()
{
    if (!m_isStreaming) return;
    
    m_isStreaming = false;
    
    // 타이머 중지
    m_connectionTimer->stop();
    m_liveBlinkTimer->stop();
    m_statusUpdateTimer->stop();
    
    // 미디어 플레이어 중지
    if (m_mediaPlayer) {
        m_mediaPlayer->stop();
    }
    
    m_liveIndicator->setVisible(false);
    showConnectionStatus("스트림 중지됨", "#666");
    
    qDebug() << "스트림 중지됨";
}

bool VideoStreamWidget::isStreaming() const
{
    return m_isStreaming;
}

void VideoStreamWidget::setStreamUrl(const QString &url)
{
    m_rtspUrl = url;
}

void VideoStreamWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit clicked();
    }
    QWidget::mousePressEvent(event);
}

void VideoStreamWidget::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    qDebug() << "미디어 상태 변경:" << status;
    
    switch (status) {
    case QMediaPlayer::LoadingMedia:
        showConnectionStatus("미디어 로딩 중...", "#ff9800");
        break;
        
    case QMediaPlayer::LoadedMedia:
        showConnectionStatus("미디어 로드 완료", "#4caf50");
        m_connectionTimer->stop();
        break;
        
    case QMediaPlayer::BufferingMedia:
        showConnectionStatus("연결됨", "#4caf50");
        m_connectionTimer->stop();
        m_liveIndicator->setVisible(true);
        m_liveBlinkTimer->start();
        m_reconnectAttempts = 0;
        QMessageBox::information(this, "RTSP 연결", "RTSP 연결에 성공했습니다!");
        break;
        
    case QMediaPlayer::BufferedMedia:
        showConnectionStatus("연결됨", "#4caf50");
        m_connectionTimer->stop();
        m_liveIndicator->setVisible(true);
        m_liveBlinkTimer->start();
        m_reconnectAttempts = 0;
        qDebug() << "버퍼링 완료 - 스트림 재생 시작";
        break;
        
    case QMediaPlayer::EndOfMedia:
        qDebug() << "스트림 종료됨";
        if (m_isStreaming) {
            QTimer::singleShot(5000, this, &VideoStreamWidget::attemptReconnection);
        }
        break;
        
    case QMediaPlayer::InvalidMedia:
        qDebug() << "잘못된 미디어";
        emit streamError("잘못된 미디어 형식입니다");
        attemptReconnection();
        break;
        
    default:
        break;
    }
}

void VideoStreamWidget::onPlaybackStateChanged(QMediaPlayer::PlaybackState state)
{
    qDebug() << "재생 상태 변경:" << state;
    
    switch (state) {
    case QMediaPlayer::PlayingState:
        showConnectionStatus("재생 중", "#4caf50");
        break;
        
    case QMediaPlayer::PausedState:
        showConnectionStatus("일시정지", "#ff9800");
        break;
        
    case QMediaPlayer::StoppedState:
        showConnectionStatus("중지됨", "#666");
        m_liveIndicator->setVisible(false);
        m_liveBlinkTimer->stop();
        break;
    }
}

void VideoStreamWidget::onErrorOccurred(QMediaPlayer::Error error, const QString &errorString)
{
    qDebug() << "미디어 플레이어 에러:" << error << errorString;
    
    QString errorMsg;
    switch (error) {
    case QMediaPlayer::NoError:
        return;
        
    case QMediaPlayer::ResourceError:
        errorMsg = "리소스 에러: " + errorString;
        break;
        
    case QMediaPlayer::FormatError:
        errorMsg = "포맷 에러: " + errorString;
        break;
        
    case QMediaPlayer::NetworkError:
        errorMsg = "네트워크 에러: " + errorString;
        break;
        
    case QMediaPlayer::AccessDeniedError:
        errorMsg = "접근 거부: " + errorString;
        break;
        
    default:
        errorMsg = "알 수 없는 에러: " + errorString;
        break;
    }
    
    showConnectionStatus("에러 발생", "#f44336");
    emit streamError(errorMsg);
    QMessageBox::warning(this, "RTSP 연결 실패", errorMsg);

    if (m_isStreaming) {
        attemptReconnection();
    }
}

void VideoStreamWidget::onConnectionTimeout()
{
    qDebug() << "연결 타임아웃";
    showConnectionStatus("연결 타임아웃", "#f44336");
    emit streamError("연결 타임아웃: 스트림에 연결할 수 없습니다");
    attemptReconnection();
}

void VideoStreamWidget::attemptReconnection()
{
    if (m_reconnectAttempts >= MAX_RECONNECT_ATTEMPTS) {
        qDebug() << "최대 재연결 시도 횟수 초과";
        stopStream();
        emit streamError("최대 재연결 시도 횟수를 초과했습니다");
        return;
    }
    
    m_reconnectAttempts++;
    showConnectionStatus(QString("재연결 시도 중... (%1/%2)").arg(m_reconnectAttempts).arg(MAX_RECONNECT_ATTEMPTS), "#ff9800");
    
    // 현재 재생 중지
    if (m_mediaPlayer) {
        m_mediaPlayer->stop();
    }
    
    // 잠시 대기 후 재연결 시도
    QTimer::singleShot(3000, this, [this]() {
        if (!m_rtspUrl.isEmpty() && m_isStreaming) {
            qDebug() << "재연결 시도:" << m_reconnectAttempts;
            m_connectionTimer->start();
            QUrl url(m_rtspUrl);
            m_mediaPlayer->setSource(url);
            m_mediaPlayer->play();
        }
    });
}

void VideoStreamWidget::updateConnectionStatus()
{
    if (!m_isStreaming) {
        return;
    }
    
    // 미디어 플레이어 상태 확인
    if (m_mediaPlayer->playbackState() == QMediaPlayer::PlayingState &&
        m_mediaPlayer->mediaStatus() == QMediaPlayer::BufferedMedia) {
        
        if (m_reconnectAttempts > 0) {
            m_reconnectAttempts = 0;
            showConnectionStatus("연결 복구됨", "#4caf50");
        }
    }
}

void VideoStreamWidget::showConnectionStatus(const QString &status, const QString &color)
{
    m_statusLabel->setText(status);
    m_statusLabel->setStyleSheet(QString("color: %1; font-size: 12px; padding: 5px;").arg(color));
}
