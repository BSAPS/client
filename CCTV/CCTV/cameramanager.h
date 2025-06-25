#ifndef CAMERAMANAGER_H
#define CAMERAMANAGER_H

// Qt 카메라 관련 헤더 파일들
#include <QObject>              // Qt 객체 기본 클래스
#include <QCamera>              // 카메라 제어 클래스
#include <QMediaCaptureSession> // 미디어 캡처 세션
#include <QImageCapture>        // 이미지 캡처 클래스
#include <QVideoWidget>         // 비디오 표시 위젯
#include <QCameraDevice>        // 카메라 디바이스 정보
#include <QMediaDevices>        // 미디어 디바이스 관리

class CameraManager : public QObject
{
    Q_OBJECT

public:
    explicit CameraManager(QObject *parent = nullptr); // 생성자
    ~CameraManager();                                   // 소멸자

    // 카메라 관련 공개 함수들
    QList<QCameraDevice> getAvailableCameras() const;   // 사용 가능한 카메라 목록 반환
    void setCamera(const QCameraDevice &cameraDevice);  // 사용할 카메라 설정
    void setVideoOutput(QVideoWidget *videoWidget);     // 비디오 출력 위젯 설정
    void startCamera();                                 // 카메라 시작
    void stopCamera();                                  // 카메라 정지
    void captureImage(const QString &fileName);         // 이미지 캡처
    
    bool isCameraActive() const;                        // 카메라 활성 상태 확인

signals:
    // 시그널 함수들 (이벤트 알림용)
    void cameraError(const QString &error);             // 카메라 에러 발생 시그널
    void imageCaptured(const QString &fileName);        // 이미지 캡처 완료 시그널

private slots:
    // 내부 슬롯 함수들 (이벤트 처리용)
    void onCameraError(QCamera::Error error);           // 카메라 에러 처리
    void onImageCaptured(int id, const QImage &image);  // 이미지 캡처 완료 처리

private:
    // 내부 멤버 변수들
    QCamera *m_camera;                                  // 카메라 객체 포인터
    QMediaCaptureSession *m_captureSession;             // 미디어 캡처 세션 포인터
    QImageCapture *m_imageCapture;                      // 이미지 캡처 객체 포인터
    QVideoWidget *m_videoOutput;                        // 비디오 출력 위젯 포인터
    QString m_pendingCaptureFileName;                   // 대기 중인 캡처 파일명
};

#endif // CAMERAMANAGER_H
