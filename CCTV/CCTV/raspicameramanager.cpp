#include "raspicameramanager.h"
#include <QDebug>               // 디버그 출력
#include <QDir>                 // 디렉토리 관리
#include <QStandardPaths>       // 표준 경로
#include <QFileInfo>            // 파일 정보
#include <QPixmap>              // 픽스맵
#include <QApplication>         // 애플리케이션

RaspiCameraManager::RaspiCameraManager(QObject *parent)
    : QObject(parent)
    , m_previewProcess(nullptr)
    , m_captureProcess(nullptr)
    , m_recordProcess(nullptr)
    , m_previewTimer(nullptr)
    , m_previewLabel(nullptr)
    , m_width(1920)                     // 기본 해상도: 1920x1080
    , m_height(1080)
    , m_brightness(50)                  // 기본 밝기: 50%
    , m_contrast(0)                     // 기본 대비: 0 (중간값)
    , m_rotation(0)                     // 기본 회전: 0도
    , m_hflip(false)                    // 수평 뒤집기 비활성화
    , m_vflip(false)                    // 수직 뒤집기 비활성화
    , m_cameraAvailable(false)
    , m_isRecording(false)
    , m_previewActive(false)
{
    // 미리보기 업데이트 타이머 설정
    m_previewTimer = new QTimer(this);
    connect(m_previewTimer, &QTimer::timeout, this, &RaspiCameraManager::onPreviewTimer);
    
    // 카메라 모듈 확인
    m_cameraAvailable = checkCameraModule();
    
    if (m_cameraAvailable) {
        qDebug() << "Raspberry Pi Camera detected and available";
    } else {
        qWarning() << "Raspberry Pi Camera not detected or not available";
    }
}

RaspiCameraManager::~RaspiCameraManager()
{
    stopPreview();                      // 미리보기 정지
    stopRecording();                    // 녹화 정지
    
    // 프로세스들 정리
    if (m_previewProcess) {
        m_previewProcess->kill();
        m_previewProcess->waitForFinished(3000);
        delete m_previewProcess;
    }
    if (m_captureProcess) {
        m_captureProcess->kill();
        m_captureProcess->waitForFinished(3000);
        delete m_captureProcess;
    }
    if (m_recordProcess) {
        m_recordProcess->kill();
        m_recordProcess->waitForFinished(3000);
        delete m_recordProcess;
    }
}

bool RaspiCameraManager::initializeCamera()
{
    QMutexLocker locker(&m_mutex);      // 스레드 안전성 보장
    
    if (!m_cameraAvailable) {
        emit errorOccurred("Camera module not available");
        return false;
    }
    
    // 카메라 모듈 재확인
    m_cameraAvailable = checkCameraModule();
    
    if (m_cameraAvailable) {
        qDebug() << "Camera initialized successfully";
        return true;
    } else {
        emit errorOccurred("Failed to initialize camera");
        return false;
    }
}

void RaspiCameraManager::startPreview(QLabel *previewLabel)
{
    if (!m_cameraAvailable) {
        emit errorOccurred("Camera not available for preview");
        return;
    }
    
    if (m_previewActive) {
        stopPreview();                  // 기존 미리보기 정지
    }
    
    m_previewLabel = previewLabel;      // 미리보기 라벨 설정
    
    // libcamera-still을 사용한 연속 캡처 (라즈베리파이 OS Bullseye 이후)
    if (m_previewProcess) {
        delete m_previewProcess;
    }
    
    m_previewProcess = new QProcess(this);
    
    // libcamera-still 명령어 구성 (미리보기용 연속 캡처)
    QStringList arguments;
    arguments << "--timeout" << "0"     // 무한 실행
              << "--preview" << QString("%1,%2,%3,%4").arg(0).arg(0).arg(m_width/4).arg(m_height/4)
              << "--width" << QString::number(m_width/4)    // 미리보기는 1/4 크기
              << "--height" << QString::number(m_height/4)
              << "--output" << "/tmp/preview_%04d.jpg"      // 임시 파일로 저장
              << "--timelapse" << "100";                    // 100ms 간격
    
    // 카메라 설정 적용
    if (m_brightness != 50) {
        arguments << "--brightness" << QString::number((m_brightness - 50) / 50.0);
    }
    if (m_contrast != 0) {
        arguments << "--contrast" << QString::number(m_contrast / 100.0);
    }
    if (m_rotation != 0) {
        arguments << "--rotation" << QString::number(m_rotation);
    }
    if (m_hflip) {
        arguments << "--hflip";
    }
    if (m_vflip) {
        arguments << "--vflip";
    }
    
    // 프로세스 시작
    m_previewProcess->start("libcamera-still", arguments);
    
    if (m_previewProcess->waitForStarted(3000)) {
        m_previewActive = true;
        m_previewTimer->start(200);     // 200ms마다 미리보기 업데이트
        qDebug() << "Preview started successfully";
    } else {
        emit errorOccurred("Failed to start camera preview");
        delete m_previewProcess;
        m_previewProcess = nullptr;
    }
}

void RaspiCameraManager::stopPreview()
{
    QMutexLocker locker(&m_mutex);
    
    if (m_previewTimer) {
        m_previewTimer->stop();         // 타이머 정지
    }
    
    if (m_previewProcess && m_previewProcess->state() == QProcess::Running) {
        m_previewProcess->terminate();  // 프로세스 종료
        if (!m_previewProcess->waitForFinished(3000)) {
            m_previewProcess->kill();   // 강제 종료
        }
    }
    
    m_previewActive = false;
    m_previewLabel = nullptr;
    
    // 임시 미리보기 파일들 정리
    QDir tempDir("/tmp");
    QStringList filters;
    filters << "preview_*.jpg";
    QFileInfoList files = tempDir.entryInfoList(filters, QDir::Files);
    for (const QFileInfo &file : files) {
        QFile::remove(file.absoluteFilePath());
    }
    
    qDebug() << "Preview stopped";
}

void RaspiCameraManager::captureImage(const QString &fileName)
{
    if (!m_cameraAvailable) {
        emit errorOccurred("Camera not available for capture");
        return;
    }
    
    // 캡처 프로세스 생성
    if (m_captureProcess) {
        delete m_captureProcess;
    }
    
    m_captureProcess = new QProcess(this);
    connect(m_captureProcess, QOverload<int>::of(&QProcess::finished),
            this, &RaspiCameraManager::onCaptureFinished);
    
    // libcamera-still 명령어 생성
    QString command = buildRaspistillCommand(fileName);
    QStringList arguments = command.split(' ', Qt::SkipEmptyParts);
    QString program = arguments.takeFirst();
    
    qDebug() << "Capturing image with command:" << program << arguments.join(' ');
    
    // 캡처 실행
    m_captureProcess->start(program, arguments);
    
    if (!m_captureProcess->waitForStarted(3000)) {
        emit errorOccurred("Failed to start image capture");
    }
}

void RaspiCameraManager::startRecording(const QString &fileName)
{
    if (!m_cameraAvailable) {
        emit errorOccurred("Camera not available for recording");
        return;
    }
    
    if (m_isRecording) {
        stopRecording();                // 기존 녹화 정지
    }
    
    // 녹화 프로세스 생성
    if (m_recordProcess) {
        delete m_recordProcess;
    }
    
    m_recordProcess = new QProcess(this);
    connect(m_recordProcess, QOverload<int>::of(&QProcess::finished),
            this, &RaspiCameraManager::onRecordingFinished);
    
    // libcamera-vid 명령어 생성
    QString command = buildRaspividCommand(fileName);
    QStringList arguments = command.split(' ', Qt::SkipEmptyParts);
    QString program = arguments.takeFirst();
    
    qDebug() << "Starting recording with command:" << program << arguments.join(' ');
    
    // 녹화 시작
    m_recordProcess->start(program, arguments);
    
    if (m_recordProcess->waitForStarted(3000)) {
        m_isRecording = true;
        emit recordingStarted();
        qDebug() << "Recording started successfully";
    } else {
        emit errorOccurred("Failed to start video recording");
    }
}

void RaspiCameraManager::stopRecording()
{
    if (!m_isRecording || !m_recordProcess) {
        return;
    }
    
    QMutexLocker locker(&m_mutex);
    
    // 녹화 프로세스 종료
    m_recordProcess->terminate();
    if (!m_recordProcess->waitForFinished(5000)) {
        m_recordProcess->kill();        // 강제 종료
    }
    
    m_isRecording = false;
    emit recordingStopped();
    qDebug() << "Recording stopped";
}

void RaspiCameraManager::setResolution(int width, int height)
{
    m_width = width;                    // 가로 해상도 설정
    m_height = height;                  // 세로 해상도 설정
    qDebug() << "Resolution set to" << width << "x" << height;
}

void RaspiCameraManager::setBrightness(int brightness)
{
    m_brightness = qBound(0, brightness, 100);  // 0-100 범위로 제한
    qDebug() << "Brightness set to" << m_brightness;
}

void RaspiCameraManager::setContrast(int contrast)
{
    m_contrast = qBound(-100, contrast, 100);   // -100~100 범위로 제한
    qDebug() << "Contrast set to" << m_contrast;
}

void RaspiCameraManager::setRotation(int rotation)
{
    // 유효한 회전 각도만 허용 (0, 90, 180, 270)
    if (rotation == 0 || rotation == 90 || rotation == 180 || rotation == 270) {
        m_rotation = rotation;
        qDebug() << "Rotation set to" << m_rotation << "degrees";
    } else {
        qWarning() << "Invalid rotation angle:" << rotation;
    }
}

void RaspiCameraManager::setFlip(bool horizontal, bool vertical)
{
    m_hflip = horizontal;               // 수평 뒤집기 설정
    m_vflip = vertical;                 // 수직 뒤집기 설정
    qDebug() << "Flip set to - Horizontal:" << m_hflip << "Vertical:" << m_vflip;
}

bool RaspiCameraManager::isCameraAvailable() const
{
    return m_cameraAvailable;           // 카메라 사용 가능 여부 반환
}

bool RaspiCameraManager::isRecording() const
{
    return m_isRecording;               // 녹화 중 여부 반환
}

bool RaspiCameraManager::isPreviewActive() const
{
    return m_previewActive;             // 미리보기 활성 상태 반환
}

void RaspiCameraManager::onPreviewTimer()
{
    if (!m_previewLabel || !m_previewActive) {
        return;
    }
    
    updatePreview();                    // 미리보기 업데이트
}

void RaspiCameraManager::onCaptureFinished(int exitCode)
{
    if (exitCode == 0) {
        qDebug() << "Image capture completed successfully";
        // 캡처된 파일명을 시그널로 전송 (실제 구현에서는 파일명 저장 필요)
        emit imageCaptured("captured_image.jpg");
    } else {
        qWarning() << "Image capture failed with exit code:" << exitCode;
        emit errorOccurred("Image capture failed");
    }
}

void RaspiCameraManager::onRecordingFinished(int exitCode)
{
    m_isRecording = false;
    
    if (exitCode == 0) {
        qDebug() << "Video recording completed successfully";
    } else {
        qWarning() << "Video recording failed with exit code:" << exitCode;
        emit errorOccurred("Video recording failed");
    }
    
    emit recordingStopped();
}

QString RaspiCameraManager::buildRaspistillCommand(const QString &fileName)
{
    QStringList command;
    command << "libcamera-still";       // 최신 라즈베리파이 OS용 명령어
    command << "--output" << fileName;  // 출력 파일명
    command << "--width" << QString::number(m_width);       // 가로 해상도
    command << "--height" << QString::number(m_height);     // 세로 해상도
    command << "--timeout" << "1000";   // 1초 대기 후 캡처
    
    // 카메라 설정 적용
    if (m_brightness != 50) {
        command << "--brightness" << QString::number((m_brightness - 50) / 50.0);
    }
    if (m_contrast != 0) {
        command << "--contrast" << QString::number(m_contrast / 100.0);
    }
    if (m_rotation != 0) {
        command << "--rotation" << QString::number(m_rotation);
    }
    if (m_hflip) {
        command << "--hflip";
    }
    if (m_vflip) {
        command << "--vflip";
    }
    
    return command.join(' ');           // 명령어 문자열 반환
}

QString RaspiCameraManager::buildRaspividCommand(const QString &fileName)
{
    QStringList command;
    command << "libcamera-vid";         // 최신 라즈베리파이 OS용 비디오 명령어
    command << "--output" << fileName;  // 출력 파일명
    command << "--width" << QString::number(m_width);       // 가로 해상도
    command << "--height" << QString::number(m_height);     // 세로 해상도
    command << "--timeout" << "0";      // 무한 녹화 (수동 정지까지)
    command << "--codec" << "h264";     // H.264 코덱 사용
    
    // 카메라 설정 적용
    if (m_brightness != 50) {
        command << "--brightness" << QString::number((m_brightness - 50) / 50.0);
    }
    if (m_contrast != 0) {
        command << "--contrast" << QString::number(m_contrast / 100.0);
    }
    if (m_rotation != 0) {
        command << "--rotation" << QString::number(m_rotation);
    }
    if (m_hflip) {
        command << "--hflip";
    }
    if (m_vflip) {
        command << "--vflip";
    }
    
    return command.join(' ');           // 명령어 문자열 반환
}

void RaspiCameraManager::updatePreview()
{
    if (!m_previewLabel) {
        return;
    }
    
    // 최신 미리보기 이미지 파일 찾기
    QDir tempDir("/tmp");
    QStringList filters;
    filters << "preview_*.jpg";
    QFileInfoList files = tempDir.entryInfoList(filters, QDir::Files, QDir::Time);
    
    if (!files.isEmpty()) {
        // 가장 최근 파일 로드
        QString latestFile = files.first().absoluteFilePath();
        QPixmap pixmap(latestFile);
        
        if (!pixmap.isNull()) {
            // 라벨 크기에 맞게 스케일링
            QPixmap scaledPixmap = pixmap.scaled(m_previewLabel->size(), 
                                               Qt::KeepAspectRatio, 
                                               Qt::SmoothTransformation);
            m_previewLabel->setPixmap(scaledPixmap);
            emit imageReady(scaledPixmap);
        }
    }
}

bool RaspiCameraManager::checkCameraModule()
{
    // vcgencmd를 사용하여 카메라 모듈 확인
    QProcess process;
    process.start("vcgencmd", QStringList() << "get_camera");
    process.waitForFinished(3000);
    
    QString output = process.readAllStandardOutput();
    
    // "supported=1 detected=1" 형태의 출력 확인
    bool supported = output.contains("supported=1");
    bool detected = output.contains("detected=1");
    
    qDebug() << "Camera check output:" << output.trimmed();
    
    return supported && detected;       // 지원되고 감지된 경우에만 true
}
