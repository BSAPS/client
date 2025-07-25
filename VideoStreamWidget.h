#ifndef VIDEOSTREAMWIDGET_H
#define VIDEOSTREAMWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QAudioOutput>
#include <QMutex>
#include <QPainter>
#include <QQueue>
#include <QDateTime>
#include <QPair>
#include <QImage>

#include "TcpCommunicator.h"
#include "BBox.h"

class VideoStreamWidget : public QWidget
{
    Q_OBJECT

public:
    explicit VideoStreamWidget(QWidget *parent = nullptr);
    ~VideoStreamWidget();

    void startStream(const QString &rtspUrl);
    void stopStream();
    bool isStreaming() const;
    void setStreamUrl(const QString &url);

    // BBox 그리는 함수
    void setFrame(const QImage &frame);
    void setBBoxes(const QList<BBox> &bboxes);
    void setBboxEnabled(bool enabled);

signals:
    void clicked();
    void streamError(const QString &error);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void onPlaybackStateChanged(QMediaPlayer::PlaybackState state);
    void onErrorOccurred(QMediaPlayer::Error error, const QString &errorString);
    void onConnectionTimeout();
    void attemptReconnection();
    void updateConnectionStatus();

private:
    void setupUI();
    void setupMediaPlayer();
    void setupTimers();
    void showConnectionStatus(const QString &status, const QString &color);

    // UI 컴포넌트
    QVideoWidget *m_videoWidget;
    QLabel *m_statusLabel;
    QLabel *m_liveIndicator;
    QVBoxLayout *m_layout;

    // 미디어 플레이어
    QMediaPlayer *m_mediaPlayer;
    QAudioOutput *m_audioOutput;

    // 타이머
    QTimer *m_connectionTimer;
    QTimer *m_liveBlinkTimer;
    QTimer *m_statusUpdateTimer;

    // 상태 변수
    QString m_rtspUrl;
    bool m_isStreaming;
    int m_reconnectAttempts;

    // 상수
    static const int MAX_RECONNECT_ATTEMPTS = 5;
    static const int CONNECTION_TIMEOUT_MS = 15000; // 15초

    // BBox 좌표 리스트
    // 프레임 및 BBox 버퍼
    QQueue<QPair<QImage, qint64>> m_frameBuffer;
    QQueue<TimestampedBBox> m_bboxBuffer;
    QList<BBox> m_currentBBoxes;
    QImage m_frame;
    bool m_bboxEnabled = false;
    QMutex m_mutex;
    
    // 버퍼 관련 상수
    static const int MAX_BUFFER_SIZE = 30;  // 약 1초치 프레임
    static const qint64 SYNC_THRESHOLD = 33;  // 33ms = 1프레임
    
    // 현재 타임스탬프
    qint64 m_currentTimestamp;
    
private slots:
    void processBuffers();
};

#endif // VIDEOSTREAMWIDGET_H
