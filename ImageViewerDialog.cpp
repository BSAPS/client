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

    setStyleSheet("background-color: #2e2e3a; color: white;");

    QHBoxLayout *headerLayout = new QHBoxLayout();

    m_timestampLabel = new QLabel();
    m_timestampLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #ffffff; padding: 10px;");
    headerLayout->addWidget(m_timestampLabel);

    headerLayout->addStretch();

    m_closeButton = new QPushButton("닫기");
    m_closeButton->setStyleSheet(R"(
        QPushButton {
            background-color: #f44336;
            color: white;
            padding: 8px 16px;
            border: none;
            border-radius: 4px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #d32f2f;
        })");
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::close);
    headerLayout->addWidget(m_closeButton);

    mainLayout->addLayout(headerLayout);

    m_scrollArea = new QScrollArea();
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setAlignment(Qt::AlignCenter);
    m_scrollArea->setStyleSheet("QScrollArea { border: none; background-color: #2e2e3a; }");

    m_imageLabel = new QLabel();
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setStyleSheet("background-color: transparent;");
    //m_imageLabel->setScaledContents(true);  // 크기 자동 조절

    m_scrollArea->setWidget(m_imageLabel);
    mainLayout->addWidget(m_scrollArea, 3);

    QLabel *logLabel = new QLabel("로그 정보:");
    logLabel->setStyleSheet("font-weight: bold; color: #ffffff; margin-top: 10px;");
    mainLayout->addWidget(logLabel);

    m_logTextEdit = new QTextEdit();
    m_logTextEdit->setMaximumHeight(150);
    m_logTextEdit->setReadOnly(true);
    m_logTextEdit->setStyleSheet(R"(
        QTextEdit {
            border: 1px solid #555;
            background-color: #1e1e2f;
            color: white;
            font-family: monospace;
            font-size: 12px;
        })");
    mainLayout->addWidget(m_logTextEdit, 1);
    setLayout(mainLayout);
}

void ImageViewerDialog::setImage(const QPixmap &pixmap, const QString &timestamp, const QString &logText)
{
    if (!pixmap.isNull()) {
        // 1. 화면 크기 정보 가져오기
        QScreen *screen = QApplication::primaryScreen();
        QRect screenGeometry = screen->availableGeometry();

        const int screenMaxWidth = screenGeometry.width() * 0.7;
        const int screenMaxHeight = screenGeometry.height() * 0.7;
        const int minWidth = 640;
        const int minHeight = 360;

        // 2. 이미지 크기 조절
        QSize originalSize = pixmap.size();
        QSize scaledSize = originalSize;

        scaledSize.scale(screenMaxWidth, screenMaxHeight, Qt::KeepAspectRatio);

        // 최소 크기 보장
        if (scaledSize.width() < minWidth || scaledSize.height() < minHeight) {
            scaledSize.setWidth(std::max(scaledSize.width(), minWidth));
            scaledSize.setHeight(std::max(scaledSize.height(), minHeight));
        }

        QPixmap scaledPixmap = pixmap.scaled(scaledSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        // 3. 이미지 적용
        m_imageLabel->setPixmap(scaledPixmap);
        m_imageLabel->resize(scaledPixmap.size());

        // 4. 창 크기 동적으로 조정
        const int headerHeight = 60; // 타이틀 + 버튼 여유
        const int logAreaHeight = 180; // 로그 영역 여유
        const int margin = 40;

        int dialogWidth = scaledSize.width() + margin;
        int dialogHeight = scaledSize.height() + headerHeight + logAreaHeight;

        resize(dialogWidth, dialogHeight);
        move((screenGeometry.width() - dialogWidth) / 2,
             (screenGeometry.height() - dialogHeight) / 2);
    } else {
        m_imageLabel->setText("이미지를 불러올 수 없습니다.");
    }

    m_timestampLabel->setText(QString("촬영 시간: %1").arg(timestamp));
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
