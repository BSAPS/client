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

/**
 * @brief RTSP 비디오 스트림 위젯 클래스
 * @details RTSP 스트림 재생, 상태 표시, 재연결 기능을 제공하는 위젯입니다.
 */
class VideoStreamWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief VideoStreamWidget 생성자
     * @param parent 부모 위젯
     */
    explicit VideoStreamWidget(QWidget *parent = nullptr);
    /**
     * @brief VideoStreamWidget 소멸자
     */
    ~VideoStreamWidget();
    /**
     * @brief 스트림 시작
     * @param rtspUrl RTSP 주소
     */
    void startStream(const QString &rtspUrl);
    /**
     * @brief 스트림 중지
     */
    void stopStream();
    /**
     * @brief 스트리밍 여부 반환
     * @return 스트리밍 중이면 true
     */
    bool isStreaming() const;
    /**
     * @brief 스트림 URL 설정
     * @param url RTSP 주소
     */
    void setStreamUrl(const QString &url);

signals:
    /** @brief 클릭 시그널 */
    void clicked();
    /** @brief draw 버튼 클릭 시그널 */
    void drawButtonClicked();
    /** @brief 스트림 에러 시그널 */
    void streamError(const QString &error);

protected:
    /**
     * @brief 마우스 클릭 이벤트 처리
     * @param event 마우스 이벤트
     */
    void mousePressEvent(QMouseEvent *event) override;

private slots:
    /**
     * @brief 미디어 상태 변경 슬롯
     * @param status 미디어 상태
     */
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);
    /**
     * @brief 재생 상태 변경 슬롯
     * @param state 재생 상태
     */
    void onPlaybackStateChanged(QMediaPlayer::PlaybackState state);
    /**
     * @brief 에러 발생 슬롯
     * @param error 에러 코드
     * @param errorString 에러 메시지
     */
    void onErrorOccurred(QMediaPlayer::Error error, const QString &errorString);
    /**
     * @brief 연결 타임아웃 슬롯
     */
    void onConnectionTimeout();
    /**
     * @brief 재연결 시도 슬롯
     */
    void attemptReconnection();
    /**
     * @brief 연결 상태 업데이트 슬롯
     */
    void updateConnectionStatus();

private:
    /** @brief UI 설정 함수 */
    void setupUI();
    /** @brief 미디어플레이어 설정 함수 */
    void setupMediaPlayer();
    /** @brief 타이머 설정 함수 */
    void setupTimers();
    /**
     * @brief 연결 상태 표시
     * @param status 상태 문자열
     * @param color 색상
     */
    void showConnectionStatus(const QString &status, const QString &color);

    /** @brief 비디오 위젯 */
    QVideoWidget *m_videoWidget;
    /** @brief 상태 라벨 */
    QLabel *m_statusLabel;
    /** @brief LIVE 인디케이터 */
    QLabel *m_liveIndicator;
    /** @brief 메인 레이아웃 */
    QVBoxLayout *m_layout;
    /** @brief 미디어 플레이어 */
    QMediaPlayer *m_mediaPlayer;
    /** @brief 연결 타이머 */
    QTimer *m_connectionTimer;
    /** @brief LIVE 인디케이터 타이머 */
    QTimer *m_liveBlinkTimer;
    /** @brief 상태 업데이트 타이머 */
    QTimer *m_statusUpdateTimer;
    /** @brief RTSP 주소 */
    QString m_rtspUrl;
    /** @brief 스트리밍 여부 */
    bool m_isStreaming;
    /** @brief 재연결 시도 횟수 */
    int m_reconnectAttempts;
    /** @brief 최대 재연결 시도 횟수 */
    static const int MAX_RECONNECT_ATTEMPTS = 5;
    /** @brief 연결 타임아웃(ms) */
    static const int CONNECTION_TIMEOUT_MS = 15000;
};

#endif // VIDEOSTREAMWIDGET_H
