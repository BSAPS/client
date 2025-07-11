#include "ImageViewerDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QApplication>
#include <QScreen>
#include <QKeyEvent>

ImageViewerDialog::ImageViewerDialog(QWidget *parent)
    : QDialog(parent)
    , m_imageLabel(nullptr)
    , m_timestampLabel(nullptr)
    , m_logTextEdit(nullptr)
    , m_closeButton(nullptr)
    , m_scrollArea(nullptr)
{
    setupUI();
    setWindowTitle("이미지 뷰어");
    setModal(true);
    
    // 화면 크기의 80%로 설정
    QScreen *screen = QApplication::primaryScreen();
    QRect screenGeometry = screen->availableGeometry();
    int width = screenGeometry.width() * 0.8;
    int height = screenGeometry.height() * 0.8;
    resize(width, height);
    
    // 화면 중앙에 배치
    move((screenGeometry.width() - width) / 2,
         (screenGeometry.height() - height) / 2);
}

ImageViewerDialog::~ImageViewerDialog()
{
}

void ImageViewerDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // 헤더 영역
    QHBoxLayout *headerLayout = new QHBoxLayout();
    
    m_timestampLabel = new QLabel();
    m_timestampLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #333; padding: 10px;");
    headerLayout->addWidget(m_timestampLabel);
    
    headerLayout->addStretch();
    
    m_closeButton = new QPushButton("닫기");
    m_closeButton->setStyleSheet("QPushButton { background-color: #f44336; color: white; padding: 8px 16px; border: none; border-radius: 4px; font-weight: bold; } QPushButton:hover { background-color: #d32f2f; }");
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::close);
    headerLayout->addWidget(m_closeButton);
    
    mainLayout->addLayout(headerLayout);
    
    // 이미지 영역
    m_scrollArea = new QScrollArea();
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setAlignment(Qt::AlignCenter);
    m_scrollArea->setStyleSheet("QScrollArea { border: 1px solid #ddd; background-color: #f9f9f9; }");
    
    m_imageLabel = new QLabel();
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setStyleSheet("background-color: white; padding: 10px;");
    m_imageLabel->setScaledContents(false);
    
    m_scrollArea->setWidget(m_imageLabel);
    mainLayout->addWidget(m_scrollArea, 3);
    
    // 로그 텍스트 영역
    QLabel *logLabel = new QLabel("로그 정보:");
    logLabel->setStyleSheet("font-weight: bold; color: #333; margin-top: 10px;");
    mainLayout->addWidget(logLabel);
    
    m_logTextEdit = new QTextEdit();
    m_logTextEdit->setMaximumHeight(150);
    m_logTextEdit->setReadOnly(true);
    m_logTextEdit->setStyleSheet("QTextEdit { border: 1px solid #ddd; background-color: #f9f9f9; font-family: monospace; font-size: 12px; }");
    mainLayout->addWidget(m_logTextEdit, 1);
    
    setLayout(mainLayout);
}

void ImageViewerDialog::setImage(const QPixmap &pixmap, const QString &timestamp, const QString &logText)
{
    // 이미지 설정
    if (!pixmap.isNull()) {
        // 스크롤 영역 크기에 맞게 이미지 크기 조정
        QSize scrollSize = m_scrollArea->size();
        QPixmap scaledPixmap = pixmap.scaled(scrollSize * 0.9, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        m_imageLabel->setPixmap(scaledPixmap);
        m_imageLabel->resize(scaledPixmap.size());
    } else {
        m_imageLabel->setText("이미지를 불러올 수 없습니다.");
    }
    
    // 타임스탬프 설정
    m_timestampLabel->setText(QString("촬영 시간: %1").arg(timestamp));
    
    // 로그 텍스트 설정
    m_logTextEdit->setPlainText(logText);
}

void ImageViewerDialog::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        close();
    } else {
        QDialog::keyPressEvent(event);
    }
}
