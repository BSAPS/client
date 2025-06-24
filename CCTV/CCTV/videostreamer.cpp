#include "videostreamer.h"
#include <QDebug>               // 디버그 출력
#include <QMutexLocker>         // 뮤텍스 로커
#include <QPixmap>              // 픽스맵
#include <QBuffer>              // 버퍼

VideoStreamer::VideoStreamer(QObject *parent)
    : QObject(parent)
    , m_displayLabel(nullptr)
    , m_displayTimer(nullptr)
    , m_frameRate(30)                   // 기본 30 FPS
    , m_maxBufferSize(10)               // 기본 10프레임 버퍼
    , m_isStreaming(false)
    , m_totalFrameCount(0)
    , m_fpsTimer(nullptr)
    , m_frameCountForFPS(0)
    , m_currentFPS(0.0)
{
    // 표시 타이머 초기화
    m_displayTimer = new QTimer(this);
    connect(m_displayTimer, &QTimer::timeout, this, &VideoStreamer::onDisplayTimer);
    
    // FPS 계산 타이머 초기화 (1초마다 FPS 계산)
    m_fpsTimer = new QTimer(this);
    connect(m_fpsTimer, &QTimer::timeout, this, &VideoStreamer::updateFPS);
    m_fpsTimer->start(1000);            // 1초 간격
    
    setFrameRate(m_frameRate);          // 프레임 레이트 설정
}

VideoStreamer::~VideoStreamer()
{
    stopStreaming();                    // 스트리밍 정지
    clearBuffer();                      // 버퍼 클리어
}

void VideoStreamer::setDisplayLabel(QLabel *label)
{
    m_displayLabel = label;             // 표시 라벨 설정
    if (m_displayLabel) {
        m_displayLabel->setScaledContents(true);    // 라벨 크기에 맞게 스케일링
        m_displayLabel->setStyleSheet("border: 1px solid gray; background-color: black;");
    }
}

void VideoStreamer::setFrameRate(int fps)
{
    m_frameRate = qMax(1, fps);         // 최소 1 FPS
    if (m_displayTimer) {
        int interval = 1000 / m_frameRate;  // 밀리초 단위 간격 계산
        m_displayTimer->setInterval(interval);
    }
    qDebug() << "Frame rate set to" << m_frameRate << "FPS";
}

void VideoStreamer::setBufferSize(int size)
{
    m_maxBufferSize = qMax(1, size);    // 최소 1프레임 버퍼
    qDebug() << "Buffer size set to" << m_maxBufferSize << "frames";
}

void VideoStreamer::startStreaming()
{
    if (m_isStreaming) {
        return;
    }
    
    if (!m_displayLabel) {
        qWarning() << "Display label not set";
        return;
    }
    
    m_isStreaming = true;
    m_displayTimer->start();            // 표시 타이머 시작
    m_totalFrameCount = 0;              // 프레임 카운터 초기화
    m_frameCountForFPS = 0;
    
    qDebug() << "Video streaming started";
    emit streamingStarted();
}

void VideoStreamer::stopStreaming()
{
    if (!m_isStreaming) {
        return;
    }
    
    m_isStreaming = false;
    m_displayTimer->stop();             // 표시 타이머 정지
    clearBuffer();                      // 버퍼 클리어
    
    qDebug() << "Video streaming stopped";
    emit streamingStopped();
}

bool VideoStreamer::isStreaming() const
{
    return m_isStreaming;               // 스트리밍 상태 반환
}

void VideoStreamer::addFrame(const QByteArray &frameData)
{
    if (!m_isStreaming || frameData.isEmpty()) {
        return;
    }
    
    QMutexLocker locker(&m_bufferMutex); // 스레드 안전성 보장
    
    // 버퍼 오버플로우 체크
    if (m_frameBuffer.size() >= m_maxBufferSize) {
        m_frameBuffer.dequeue();        // 가장 오래된 프레임 제거
        emit bufferOverflow();          // 오버플로우 시그널
    }
    
    m_frameBuffer.enqueue(frameData);   // 새 프레임 추가
    m_totalFrameCount++;                // 총 프레임 수 증가
}

void VideoStreamer::clearBuffer()
{
    QMutexLocker locker(&m_bufferMutex);
    m_frameBuffer.clear();              // 버퍼 클리어
}

int VideoStreamer::getFrameCount() const
{
    return m_totalFrameCount;           // 총 프레임 수 반환
}

int VideoStreamer::getBufferedFrames() const
{
    QMutexLocker locker(const_cast<QMutex*>(&m_bufferMutex));
    return m_frameBuffer.size();        // 버퍼된 프레임 수 반환
}

double VideoStreamer::getCurrentFPS() const
{
    return m_currentFPS;                // 현재 FPS 반환
}

void VideoStreamer::onDisplayTimer()
{
    displayNextFrame();                 // 다음 프레임 표시
}

void VideoStreamer::displayNextFrame()
{
    if (!m_displayLabel || !m_isStreaming) {
        return;
    }
    
    QByteArray frameData;
    
    // 버퍼에서 프레임 가져오기
    {
        QMutexLocker locker(&m_bufferMutex);
        if (m_frameBuffer.isEmpty()) {
            return;                     // 표시할 프레임이 없음
        }
        frameData = m_frameBuffer.dequeue();
    }
    
    // 픽스맵 생성 및 표시
    QPixmap pixmap = createPixmapFromData(frameData);
    if (!pixmap.isNull()) {
        // 라벨 크기에 맞게 스케일링
        QPixmap scaledPixmap = pixmap.scaled(m_displayLabel->size(), 
                                           Qt::KeepAspectRatio, 
                                           Qt::SmoothTransformation);
        m_displayLabel->setPixmap(scaledPixmap);
        
        m_frameCountForFPS++;           // FPS 계산용 프레임 수 증가
        emit frameDisplayed();          // 프레임 표시 완료 시그널
    }
}

QPixmap VideoStreamer::createPixmapFromData(const QByteArray &data)
{
    QPixmap pixmap;
    
    // 바이트 배열에서 픽스맵 로드
    if (!pixmap.loadFromData(data)) {
        qWarning() << "Failed to create pixmap from frame data";
        return QPixmap();
    }
    
    return pixmap;
}

void VideoStreamer::updateFPS()
{
    m_currentFPS = m_frameCountForFPS;  // 1초간 표시된 프레임 수 = FPS
    m_frameCountForFPS = 0;             // 카운터 초기화
}
