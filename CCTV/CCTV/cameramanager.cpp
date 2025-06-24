#include "cameramanager.h"
#include <QDebug>               // 디버그 출력용
#include <QDir>                 // 디렉토리 관리
#include <QStandardPaths>       // 표준 경로 제공

CameraManager::CameraManager(QObject *parent)
    : QObject(parent)
    // 모든 포인터를 nullptr로 초기화
    , m_camera(nullptr)
    , m_captureSession(nullptr)
    , m_imageCapture(nullptr)
    , m_videoOutput(nullptr)
{
    // 미디어 캡처 세션 생성
    m_captureSession = new QMediaCaptureSession(this);
    // 이미지 캡처 객체 생성
    m_imageCapture = new QImageCapture(this);
    
    // 캡처 세션에 이미지 캡처 설정
    m_captureSession->setImageCapture(m_imageCapture);
    
    // 이미지 캡처 완료 시그널과 슬롯 연결
    connect(m_imageCapture, &QImageCapture::imageCaptured,
            this, &CameraManager::onImageCaptured);
}

CameraManager::~CameraManager()
{
    stopCamera();               // 소멸자에서 카메라 정지
}

QList<QCameraDevice> CameraManager::getAvailableCameras() const
{
    // 시스템에서 사용 가능한 비디오 입력 디바이스 목록 반환
    return QMediaDevices::videoInputs();
}

void CameraManager::setCamera(const QCameraDevice &cameraDevice)
{
    // 기존 카메라가 있으면 정리
    if (m_camera) {
        m_camera->stop();       // 카메라 정지
        delete m_camera;        // 메모리 해제
    }
    
    // 새로운 카메라 객체 생성
    m_camera = new QCamera(cameraDevice, this);
    // 캡처 세션에 카메라 설정
    m_captureSession->setCamera(m_camera);
    
    // 카메라 에러 시그널과 슬롯 연결
    connect(m_camera, &QCamera::errorOccurred,
            this, &CameraManager::onCameraError);
}

void CameraManager::setVideoOutput(QVideoWidget *videoWidget)
{
    m_videoOutput = videoWidget;                        // 비디오 출력 위젯 저장
    if (m_captureSession) {
        // 캡처 세션에 비디오 출력 설정
        m_captureSession->setVideoOutput(videoWidget);
    }
}

void CameraManager::startCamera()
{
    if (m_camera) {
        m_camera->start();      // 카메라 시작
    }
}

void CameraManager::stopCamera()
{
    if (m_camera) {
        m_camera->stop();       // 카메라 정지
    }
}

void CameraManager::captureImage(const QString &fileName)
{
    // 이미지 캡처 객체나 카메라가 없거나 비활성 상태면 에러 처리
    if (!m_imageCapture || !m_camera || !m_camera->isActive()) {
        emit cameraError("Camera not active for image capture");    // 에러 시그널 발생
        return;
    }
    
    m_pendingCaptureFileName = fileName;                // 캡처할 파일명 저장
    m_imageCapture->capture();                          // 이미지 캡처 실행
}

bool CameraManager::isCameraActive() const
{
    // 카메라가 존재하고 활성 상태인지 확인
    return m_camera && m_camera->isActive();
}

void CameraManager::onCameraError(QCamera::Error error)
{
    QString errorString;        // 에러 메시지 문자열
    switch (error) {            // 에러 타입에 따른 메시지 설정
    case QCamera::NoError:
        return;                 // 에러가 없으면 리턴
    case QCamera::CameraError:
        errorString = "Camera error occurred";          // 카메라 에러 메시지
        break;
    default:
        errorString = "Unknown camera error";           // 알 수 없는 에러 메시지
        break;
    }
    
    emit cameraError(errorString);                      // 에러 시그널 발생
}

void CameraManager::onImageCaptured(int id, const QImage &image)
{
    Q_UNUSED(id)                // id 매개변수 사용하지 않음을 명시
    
    // 대기 중인 파일명이 있고 이미지가 유효한 경우
    if (!m_pendingCaptureFileName.isEmpty() && !image.isNull()) {
        // 파일 정보 객체 생성
        QFileInfo fileInfo(m_pendingCaptureFileName);
        QDir dir = fileInfo.absoluteDir();              // 디렉토리 경로 가져오기
        if (!dir.exists()) {                            // 디렉토리가 존재하지 않으면
            dir.mkpath(".");                            // 디렉토리 생성
        }
        
        // 이미지를 파일로 저장
        if (image.save(m_pendingCaptureFileName)) {
            emit imageCaptured(m_pendingCaptureFileName); // 캡처 완료 시그널 발생
        }
        
        m_pendingCaptureFileName.clear();               // 대기 중인 파일명 초기화
    }
}
