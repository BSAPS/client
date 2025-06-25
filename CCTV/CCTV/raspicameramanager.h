#ifndef RASPICAMERAMANAGER_H
#define RASPICAMERAMANAGER_H

#include <QObject>              // Qt 객체 기본 클래스
#include <QProcess>             // 외부 프로세스 실행
#include <QTimer>               // 타이머 클래스
#include <QLabel>               // 라벨 위젯
#include <QPixmap>              // 픽스맵 클래스
#include <QThread>              // 스레드 클래스
#include <QMutex>               // 뮤텍스 클래스
#include <QVideoWidget>         // 비디오 위젯

// 라즈베리파이 카메라 전용 관리 클래스
class RaspiCameraManager : public QObject
{
    Q_OBJECT

public:
    explicit RaspiCameraManager(QObject *parent = nullptr);  // 생성자
    ~RaspiCameraManager();                                   // 소멸자

    // 카메라 제어 함수들
    bool initializeCamera();                                 // 카메라 초기화
    void startPreview(QLabel *previewLabel);                 // 미리보기 시작
    void stopPreview();                                      // 미리보기 정지
    void captureImage(const QString &fileName);              // 이미지 캡처
    void startRecording(const QString &fileName);            // 비디오 녹화 시작
    void stopRecording();                                    // 비디오 녹화 정지
    
    // 카메라 설정 함수들
    void setResolution(int width, int height);               // 해상도 설정
    void setBrightness(int brightness);                      // 밝기 설정 (0-100)
    void setContrast(int contrast);                          // 대비 설정 (-100 ~ 100)
    void setRotation(int rotation);                          // 회전 설정 (0, 90, 180, 270)
    void setFlip(bool horizontal, bool vertical);            // 뒤집기 설정
    
    // 상태 확인 함수들
    bool isCameraAvailable() const;                          // 카메라 사용 가능 여부
    bool isRecording() const;                                // 녹화 중인지 확인
    bool isPreviewActive() const;                            // 미리보기 활성 상태

signals:
    void imageReady(const QPixmap &image);                   // 이미지 준비 완료 시그널
    void imageCaptured(const QString &fileName);             // 이미지 캡처 완료 시그널
    void recordingStarted();                                 // 녹화 시작 시그널
    void recordingStopped();                                 // 녹화 정지 시그널
    void errorOccurred(const QString &error);                // 에러 발생 시그널

private slots:
    void onPreviewTimer();                                   // 미리보기 타이머 슬롯
    void onCaptureFinished(int exitCode);                    // 캡처 완료 슬롯
    void onRecordingFinished(int exitCode);                  // 녹화 완료 슬롯

private:
    // 내부 함수들
    QString buildRaspistillCommand(const QString &fileName); // raspistill 명령어 생성
    QString buildRaspividCommand(const QString &fileName);   // raspivid 명령어 생성
    void updatePreview();                                    // 미리보기 업데이트
    bool checkCameraModule();                                // 카메라 모듈 확인

    // 멤버 변수들
    QProcess *m_previewProcess;                              // 미리보기 프로세스
    QProcess *m_captureProcess;                              // 캡처 프로세스
    QProcess *m_recordProcess;                               // 녹화 프로세스
    QTimer *m_previewTimer;                                  // 미리보기 업데이트 타이머
    QLabel *m_previewLabel;                                  // 미리보기 표시 라벨
    
    // 카메라 설정 변수들
    int m_width;                                             // 가로 해상도
    int m_height;                                            // 세로 해상도
    int m_brightness;                                        // 밝기 (0-100)
    int m_contrast;                                          // 대비 (-100~100)
    int m_rotation;                                          // 회전 각도
    bool m_hflip;                                            // 수평 뒤집기
    bool m_vflip;                                            // 수직 뒤집기
    
    // 상태 변수들
    bool m_cameraAvailable;                                  // 카메라 사용 가능 여부
    bool m_isRecording;                                      // 녹화 중 여부
    bool m_previewActive;                                    // 미리보기 활성 여부
    
    QMutex m_mutex;                                          // 스레드 안전성을 위한 뮤텍스
};

#endif // RASPICAMERAMANAGER_H
