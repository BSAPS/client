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

signals:
    void clicked();
    void streamError(const QString &error);

protected:
    void mousePressEvent(QMouseEvent *event) override;

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
};

#endif // VIDEOSTREAMWIDGET_H
