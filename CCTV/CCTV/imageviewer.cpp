#include "imageviewer.h"
#include <QDebug>               // 디버그 출력
#include <QFileDialog>          // 파일 대화상자
#include <QMessageBox>          // 메시지 박스
#include <QApplication>         // 애플리케이션

ImageViewer::ImageViewer(QWidget *parent)
    : QWidget(parent)
    , m_mainLayout(nullptr)
    , m_controlLayout(nullptr)
    , m_scrollArea(nullptr)
    , m_imageLabel(nullptr)
    , m_previousButton(nullptr)
    , m_nextButton(nullptr)
    , m_zoomInButton(nullptr)
    , m_zoomOutButton(nullptr)
    , m_resetZoomButton(nullptr)
    , m_fitToWindowButton(nullptr)
    , m_saveButton(nullptr)
    , m_zoomSlider(nullptr)
    , m_infoLabel(nullptr)
    , m_currentIndex(-1)
    , m_zoomFactor(1.0)
{
    setupUI();                          // UI 구성
    updateUI();                         // UI 상태 업데이트
}

ImageViewer::~ImageViewer()
{
    clearImages();                      // 이미지 클리어
}

void ImageViewer::addImage(const QByteArray &imageData)
{
    QPixmap pixmap;
    if (pixmap.loadFromData(imageData)) {   // 바이트 배열에서 픽스맵 로드
        addImage(pixmap);               // 픽스맵 추가
    } else {
        qWarning() << "Failed to load image from data";
    }
}

void ImageViewer::addImage(const QPixmap &pixmap)
{
    if (pixmap.isNull()) {
        qWarning() << "Cannot add null pixmap";
        return;
    }
    
    m_images.append(pixmap);            // 이미지 리스트에 추가
    
    // 첫 번째 이미지인 경우 표시
    if (m_images.size() == 1) {
        m_currentIndex = 0;
        showImage(0);
    }
    
    updateUI();                         // UI 업데이트
    emit imageAdded(m_images.size());   // 이미지 추가 시그널
    
    qDebug() << "Image added, total count:" << m_images.size();
}

void ImageViewer::clearImages()
{
    m_images.clear();                   // 이미지 리스트 클리어
    m_currentIndex = -1;
    m_originalPixmap = QPixmap();
    m_imageLabel->clear();              // 라벨 클리어
    updateUI();                         // UI 업데이트
    
    qDebug() << "All images cleared";
}

void ImageViewer::saveCurrentImage(const QString &fileName)
{
    if (m_currentIndex < 0 || m_currentIndex >= m_images.size()) {
        qWarning() << "No current image to save";
        return;
    }
    
    const QPixmap &currentImage = m_images[m_currentIndex];
    if (currentImage.save(fileName)) {
        qDebug() << "Image saved to:" << fileName;
    } else {
        qWarning() << "Failed to save image to:" << fileName;
    }
}

void ImageViewer::showImage(int index)
{
    if (index < 0 || index >= m_images.size()) {
        return;
    }
    
    m_currentIndex = index;
    m_originalPixmap = m_images[index]; // 원본 픽스맵 저장
    updateImageDisplay();               // 이미지 표시 업데이트
    updateUI();                         // UI 업데이트
    
    emit imageChanged(index);           // 이미지 변경 시그널
}

void ImageViewer::showNextImage()
{
    if (m_currentIndex < m_images.size() - 1) {
        showImage(m_currentIndex + 1);  // 다음 이미지 표시
    }
}

void ImageViewer::showPreviousImage()
{
    if (m_currentIndex > 0) {
        showImage(m_currentIndex - 1);  // 이전 이미지 표시
    }
}

void ImageViewer::showFirstImage()
{
    if (!m_images.isEmpty()) {
        showImage(0);                   // 첫 번째 이미지 표시
    }
}

void ImageViewer::showLastImage()
{
    if (!m_images.isEmpty()) {
        showImage(m_images.size() - 1); // 마지막 이미지 표시
    }
}

void ImageViewer::zoomIn()
{
    setZoomFactor(m_zoomFactor * 1.25); // 25% 확대
}

void ImageViewer::zoomOut()
{
    setZoomFactor(m_zoomFactor / 1.25); // 25% 축소
}

void ImageViewer::resetZoom()
{
    setZoomFactor(1.0);                 // 원본 크기
}

void ImageViewer::fitToWindow()
{
    if (m_originalPixmap.isNull() || !m_scrollArea) {
        return;
    }
    
    // 스크롤 영역 크기에 맞는 비율 계산
    QSize scrollSize = m_scrollArea->size();
    QSize pixmapSize = m_originalPixmap.size();
    
    double widthRatio = (double)scrollSize.width() / pixmapSize.width();
    double heightRatio = (double)scrollSize.height() / pixmapSize.height();
    double fitRatio = qMin(widthRatio, heightRatio);
    
    setZoomFactor(fitRatio);            // 맞춤 비율로 설정
}

int ImageViewer::getImageCount() const
{
    return m_images.size();             // 이미지 개수 반환
}

int ImageViewer::getCurrentIndex() const
{
    return m_currentIndex;              // 현재 인덱스 반환
}

double ImageViewer::getZoomFactor() const
{
    return m_zoomFactor;                // 현재 확대 비율 반환
}

void ImageViewer::setupUI()
{
    // 메인 레이아웃 생성
    m_mainLayout = new QVBoxLayout(this);
    
    // 이미지 표시 영역 설정
    m_scrollArea = new QScrollArea(this);
    m_imageLabel = new QLabel();
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setStyleSheet("background-color: #f0f0f0; border: 1px solid gray;");
    m_imageLabel->setMinimumSize(400, 300);
    m_scrollArea->setWidget(m_imageLabel);
    m_scrollArea->setWidgetResizable(false);
    
    // 컨트롤 버튼들 생성
    m_controlLayout = new QHBoxLayout();
    
    m_previousButton = new QPushButton("◀ Previous", this);
    m_nextButton = new QPushButton("Next ▶", this);
    m_zoomInButton = new QPushButton("Zoom In", this);
    m_zoomOutButton = new QPushButton("Zoom Out", this);
    m_resetZoomButton = new QPushButton("Reset", this);
    m_fitToWindowButton = new QPushButton("Fit Window", this);
    m_saveButton = new QPushButton("Save", this);
    
    // 확대 슬라이더 생성
    m_zoomSlider = new QSlider(Qt::Horizontal, this);
    m_zoomSlider->setRange(10, 500);    // 10% ~ 500%
    m_zoomSlider->setValue(100);        // 기본 100%
    m_zoomSlider->setFixedWidth(150);
    
    // 정보 라벨 생성
    m_infoLabel = new QLabel("No images", this);
    
    // 컨트롤 레이아웃에 위젯들 추가
    m_controlLayout->addWidget(m_previousButton);
    m_controlLayout->addWidget(m_nextButton);
    m_controlLayout->addWidget(m_zoomInButton);
    m_controlLayout->addWidget(m_zoomOutButton);
    m_controlLayout->addWidget(m_resetZoomButton);
    m_controlLayout->addWidget(m_fitToWindowButton);
    m_controlLayout->addWidget(m_zoomSlider);
    m_controlLayout->addWidget(m_saveButton);
    m_controlLayout->addStretch();
    m_controlLayout->addWidget(m_infoLabel);
    
    // 메인 레이아웃에 추가
    m_mainLayout->addWidget(m_scrollArea);
    m_mainLayout->addLayout(m_controlLayout);
    
    // 시그널 연결
    connect(m_previousButton, &QPushButton::clicked, this, &ImageViewer::onPreviousClicked);
    connect(m_nextButton, &QPushButton::clicked, this, &ImageViewer::onNextClicked);
    connect(m_zoomInButton, &QPushButton::clicked, this, &ImageViewer::onZoomInClicked);
    connect(m_zoomOutButton, &QPushButton::clicked, this, &ImageViewer::onZoomOutClicked);
    connect(m_resetZoomButton, &QPushButton::clicked, this, &ImageViewer::onResetZoomClicked);
    connect(m_fitToWindowButton, &QPushButton::clicked, this, &ImageViewer::onFitToWindowClicked);
    connect(m_saveButton, &QPushButton::clicked, this, &ImageViewer::onSaveClicked);
    connect(m_zoomSlider, &QSlider::valueChanged, this, &ImageViewer::onZoomSliderChanged);
}

void ImageViewer::updateUI()
{
    bool hasImages = !m_images.isEmpty();
    bool hasCurrentImage = (m_currentIndex >= 0 && m_currentIndex < m_images.size());
    
    // 버튼 활성화/비활성화
    m_previousButton->setEnabled(hasImages && m_currentIndex > 0);
    m_nextButton->setEnabled(hasImages && m_currentIndex < m_images.size() - 1);
    m_zoomInButton->setEnabled(hasCurrentImage);
    m_zoomOutButton->setEnabled(hasCurrentImage);
    m_resetZoomButton->setEnabled(hasCurrentImage);
    m_fitToWindowButton->setEnabled(hasCurrentImage);
    m_saveButton->setEnabled(hasCurrentImage);
    m_zoomSlider->setEnabled(hasCurrentImage);
    
    // 정보 라벨 업데이트
    if (hasImages) {
        QString info = QString("Image %1 of %2 (%.0f%%)")
                      .arg(m_currentIndex + 1)
                      .arg(m_images.size())
                      .arg(m_zoomFactor * 100);
        m_infoLabel->setText(info);
    } else {
        m_infoLabel->setText("No images");
    }
}

void ImageViewer::updateImageDisplay()
{
    if (m_originalPixmap.isNull()) {
        m_imageLabel->clear();
        return;
    }
    
    // 확대/축소된 픽스맵 생성
    QPixmap scaledPixmap = scalePixmap(m_originalPixmap, m_zoomFactor);
    m_imageLabel->setPixmap(scaledPixmap);
    m_imageLabel->resize(scaledPixmap.size());
}

void ImageViewer::onPreviousClicked()
{
    showPreviousImage();                // 이전 이미지 표시
}

void ImageViewer::onNextClicked()
{
    showNextImage();                    // 다음 이미지 표시
}

void ImageViewer::onZoomInClicked()
{
    zoomIn();                           // 확대
}

void ImageViewer::onZoomOutClicked()
{
    zoomOut();                          // 축소
}

void ImageViewer::onResetZoomClicked()
{
    resetZoom();                        // 원본 크기
}

void ImageViewer::onFitToWindowClicked()
{
    fitToWindow();                      // 창 크기에 맞춤
}

void ImageViewer::onSaveClicked()
{
    if (m_currentIndex < 0 || m_currentIndex >= m_images.size()) {
        return;
    }
    
    // 파일 저장 대화상자
    QString fileName = QFileDialog::getSaveFileName(this,
        "Save Image", QString("image_%1.jpg").arg(m_currentIndex + 1),
        "Images (*.png *.jpg *.jpeg *.bmp)");
    
    if (!fileName.isEmpty()) {
        saveCurrentImage(fileName);     // 이미지 저장
    }
}

void ImageViewer::onZoomSliderChanged(int value)
{
    double factor = value / 100.0;      // 슬라이더 값을 비율로 변환
    setZoomFactor(factor);
}

QPixmap ImageViewer::scalePixmap(const QPixmap &pixmap, double factor)
{
    if (pixmap.isNull() || factor <= 0) {
        return QPixmap();
    }
    
    QSize newSize = pixmap.size() * factor;
    return pixmap.scaled(newSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

void ImageViewer::setZoomFactor(double factor)
{
    factor = qBound(0.1, factor, 5.0);  // 10% ~ 500% 범위로 제한
    
    if (qAbs(factor - m_zoomFactor) < 0.01) {
        return;                         // 변화가 미미하면 무시
    }
    
    m_zoomFactor = factor;
    m_zoomSlider->setValue(qRound(factor * 100));  // 슬라이더 업데이트
    
    updateImageDisplay();               // 이미지 표시 업데이트
    updateUI();                         // UI 업데이트
    
    emit zoomChanged(factor);           // 확대 비율 변경 시그널
}
