#ifndef VIDEOSTREAMER_H
#define VIDEOSTREAMER_H

#include <QObject>              // Qt 객체 기본 클래스
#include <QLabel>               // 라벨 위젯
#include <QPixmap>              // 픽스맵 클래스
#include <QTimer>               // 타이머 클래스
#include <QMutex>               // 뮤텍스 클래스
#include <QQueue>               // 큐 컨테이너

// 비디오 스트리밍 표시 클래스
class VideoStreamer : public QObject
{
    Q_OBJECT

public:
    explicit VideoStreamer(QObject *parent = nullptr);   // 생성자
    ~VideoStreamer();                                    // 소멸자

    // 비디오 표시 설정
    void setDisplayLabel(QLabel *label);                 // 표시할 라벨 설정
    void setFrameRate(int fps);                          // 프레임 레이트 설정
    void setBufferSize(int size);                        // 버퍼 크기 설정

    // 스트리밍 제어
    void startStreaming();                               // 스트리밍 시작
    void stopStreaming();                                // 스트리밍 정지
    bool isStreaming() const;                            // 스트리밍 상태 확인

    // 프레임 데이터 처리
    void addFrame(const QByteArray &frameData);          // 프레임 데이터 추가
    void clearBuffer();                                  // 버퍼 클리어

    // 통계 정보
    int getFrameCount() const;                           // 총 프레임 수
    int getBufferedFrames() const;                       // 버퍼된 프레임 수
    double getCurrentFPS() const;                        // 현재 FPS

signals:
    void frameDisplayed();                               // 프레임 표시 완료 시그널
    void bufferOverflow();                               // 버퍼 오버플로우 시그널
    void streamingStarted();                             // 스트리밍 시작 시그널
    void streamingStopped();                             // 스트리밍 정지 시그널

private slots:
    void onDisplayTimer();                               // 표시 타이머 슬롯

private:
    // 내부 함수들
    void displayNextFrame();                             // 다음 프레임 표시
    QPixmap createPixmapFromData(const QByteArray &data); // 데이터에서 픽스맵 생성
    void updateFPS();                                    // FPS 업데이트

    // 멤버 변수들
    QLabel *m_displayLabel;                              // 표시 라벨
    QTimer *m_displayTimer;                              // 표시 타이머
    QQueue<QByteArray> m_frameBuffer;                    // 프레임 버퍼
    QMutex m_bufferMutex;                                // 버퍼 뮤텍스
    
    // 설정 변수들
    int m_frameRate;                                     // 프레임 레이트 (FPS)
    int m_maxBufferSize;                                 // 최대 버퍼 크기
    
    // 상태 변수들
    bool m_isStreaming;                                  // 스트리밍 상태
    int m_totalFrameCount;                               // 총 프레임 수
    
    // FPS 계산용 변수들
    QTimer *m_fpsTimer;                                  // FPS 계산 타이머
    int m_frameCountForFPS;                              // FPS 계산용 프레임 수
    double m_currentFPS;                                 // 현재 FPS
};

#endif // VIDEOSTREAMER_H
