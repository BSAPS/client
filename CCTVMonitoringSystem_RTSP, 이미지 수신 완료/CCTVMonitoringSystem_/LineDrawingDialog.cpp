#include "LineDrawingDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QApplication>
#include <QScreen>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QDebug>
#include <QResizeEvent>

LineDrawingDialog::LineDrawingDialog(QWidget *parent)
    : QDialog(parent)
    , m_videoWidget(nullptr)
    , m_mediaPlayer(nullptr)
    , m_audioOutput(nullptr)
    , m_instructionLabel(nullptr)
    , m_confirmButton(nullptr)
    , m_cancelButton(nullptr)
    , m_isDrawing(false)
    , m_lineDrawn(false)
{
    setupUI();
    setupMediaPlayer();
    setWindowTitle("기준선 그리기");
    setModal(true);

    // 화면 크기의 90%로 설정
    QScreen *screen = QApplication::primaryScreen();
    QRect screenGeometry = screen->availableGeometry();
    int width = screenGeometry.width() * 0.9;
    int height = screenGeometry.height() * 0.9;
    resize(width, height);

    // 화면 중앙에 배치
    move((screenGeometry.width() - width) / 2,
         (screenGeometry.height() - height) / 2);
}

LineDrawingDialog::~LineDrawingDialog()
{
    if (m_mediaPlayer) {
        m_mediaPlayer->stop();
        delete m_mediaPlayer;
    }
    if (m_audioOutput) {
        delete m_audioOutput;
    }
}

void LineDrawingDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // 안내 텍스트
    m_instructionLabel = new QLabel("비디오 화면에서 마우스로 기준선을 그어주세요.");
    m_instructionLabel->setStyleSheet("font-size: 14px; color: #333; padding: 10px; background-color: #e3f2fd; border-radius: 5px; margin-bottom: 10px;");
    m_instructionLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_instructionLabel);

    // 비디오 위젯
    m_videoWidget = new ClickableVideoWidget();
    m_videoWidget->setMinimumSize(800, 600);
    m_videoWidget->setStyleSheet("border: 2px solid #ddd; background-color: #000000;");

    connect(m_videoWidget, &ClickableVideoWidget::mousePressed, this, &LineDrawingDialog::onVideoMousePressed);
    connect(m_videoWidget, &ClickableVideoWidget::mouseMoved, this, &LineDrawingDialog::onVideoMouseMoved);
    connect(m_videoWidget, &ClickableVideoWidget::mouseReleased, this, &LineDrawingDialog::onVideoMouseReleased);

    mainLayout->addWidget(m_videoWidget);

    // 버튼 영역
    QHBoxLayout *buttonLayout = new QHBoxLayout();

    buttonLayout->addStretch();

    m_confirmButton = new QPushButton("확인");
    m_confirmButton->setEnabled(false);
    m_confirmButton->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; padding: 10px 20px; border: none; border-radius: 5px; font-weight: bold; } "
                                   "QPushButton:hover { background-color: #45a049; } "
                                   "QPushButton:disabled { background-color: #cccccc; }");
    connect(m_confirmButton, &QPushButton::clicked, this, &LineDrawingDialog::onConfirmClicked);
    buttonLayout->addWidget(m_confirmButton);

    m_cancelButton = new QPushButton("취소");
    m_cancelButton->setStyleSheet("QPushButton { background-color: #f44336; color: white; padding: 10px 20px; border: none; border-radius: 5px; font-weight: bold; } QPushButton:hover { background-color: #d32f2f; }");
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(m_cancelButton);

    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);
}

void LineDrawingDialog::setupMediaPlayer()
{
    m_mediaPlayer = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);

    m_mediaPlayer->setAudioOutput(m_audioOutput);
    m_mediaPlayer->setVideoOutput(m_videoWidget);
}

void LineDrawingDialog::setStreamUrl(const QString &url)
{
    m_streamUrl = url;

    if (m_mediaPlayer && !url.isEmpty()) {
        QUrl streamUrl(url);
        m_mediaPlayer->setSource(streamUrl);
        m_mediaPlayer->play();
    }
}

void LineDrawingDialog::onVideoMousePressed(const QPoint &pos)
{
    if (!m_isDrawing) {
        m_isDrawing = true;
        m_lineDrawn = false;
        m_startPoint = pos;
        m_endPoint = pos;
        m_videoWidget->setLine(m_startPoint, m_endPoint);
        m_confirmButton->setEnabled(false);

        m_instructionLabel->setText("마우스를 드래그하여 기준선을 그어주세요.");
    }
}

void LineDrawingDialog::onVideoMouseMoved(const QPoint &pos)
{
    if (m_isDrawing) {
        m_endPoint = pos;
        m_videoWidget->setLine(m_startPoint, m_endPoint);
    }
}

void LineDrawingDialog::onVideoMouseReleased(const QPoint &pos)
{
    if (m_isDrawing) {
        m_isDrawing = false;
        m_lineDrawn = true;
        m_endPoint = pos;
        m_videoWidget->setLine(m_startPoint, m_endPoint);
        m_confirmButton->setEnabled(true);

        m_instructionLabel->setText(QString("기준선이 그어졌습니다. 확인 버튼을 눌러주세요. 시작: (%1, %2) → 끝: (%3, %4)")
                                        .arg(m_startPoint.x()).arg(m_startPoint.y())
                                        .arg(m_endPoint.x()).arg(m_endPoint.y()));
    }
}

void LineDrawingDialog::onConfirmClicked()
{
    if (m_lineDrawn) {
        emit lineDrawn(m_startPoint, m_endPoint);
        accept();
    }
}

void LineDrawingDialog::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        reject();
    } else {
        QDialog::keyPressEvent(event);
    }
}

// VideoOverlayWidget 구현
VideoOverlayWidget::VideoOverlayWidget(QWidget *parent)
    : QWidget(parent)
    , m_hasLine(false)
{
    setMouseTracking(true);
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setStyleSheet("background: transparent;");
}

void VideoOverlayWidget::setLine(const QPoint &start, const QPoint &end)
{
    m_lineStart = start;
    m_lineEnd = end;
    m_hasLine = true;
    update();
}

void VideoOverlayWidget::clearLine()
{
    m_hasLine = false;
    update();
}

void VideoOverlayWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit mousePressed(event->pos());
    }
    QWidget::mousePressEvent(event);
}

void VideoOverlayWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        emit mouseMoved(event->pos());
    }
    QWidget::mouseMoveEvent(event);
}

void VideoOverlayWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit mouseReleased(event->pos());
    }
    QWidget::mouseReleaseEvent(event);
}

void VideoOverlayWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    if (m_hasLine) {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        // 기준선을 더 눈에 띄게 그리기
        QPen linePen(QColor(255, 0, 0), 5, Qt::SolidLine); // 빨간색, 두께 5
        painter.setPen(linePen);
        painter.drawLine(m_lineStart, m_lineEnd);

        // 시작점과 끝점에 더 큰 원 그리기
        painter.setBrush(QBrush(QColor(255, 255, 0))); // 노란색 원
        painter.setPen(QPen(QColor(255, 0, 0), 3)); // 빨간색 테두리

        // 시작점 원 (16x16 크기)
        painter.drawEllipse(m_lineStart.x() - 8, m_lineStart.y() - 8, 16, 16);

        // 끝점 원 (16x16 크기)
        painter.drawEllipse(m_lineEnd.x() - 8, m_lineEnd.y() - 8, 16, 16);

        // 선 위에 텍스트 표시
        QPoint midPoint = QPoint((m_lineStart.x() + m_lineEnd.x()) / 2,
                                 (m_lineStart.y() + m_lineEnd.y()) / 2);

        painter.setPen(QPen(QColor(255, 255, 255), 2));
        painter.setFont(QFont("Arial", 12, QFont::Bold));

        // 텍스트 배경을 위한 반투명 사각형
        QString text = "기준선";
        QFontMetrics fm(painter.font());
        QRect textRect = fm.boundingRect(text);
        textRect.moveCenter(midPoint);
        textRect.adjust(-8, -4, 8, 4);

        painter.fillRect(textRect, QColor(0, 0, 0, 180)); // 반투명 검은색 배경
        painter.drawText(textRect, Qt::AlignCenter, text);
    }
}

// ClickableVideoWidget 구현
ClickableVideoWidget::ClickableVideoWidget(QWidget *parent)
    : QVideoWidget(parent)
{
    // 오버레이 위젯 생성
    m_overlayWidget = new VideoOverlayWidget(this);

    // 오버레이 위젯의 시그널을 부모로 전달
    connect(m_overlayWidget, &VideoOverlayWidget::mousePressed, this, &ClickableVideoWidget::mousePressed);
    connect(m_overlayWidget, &VideoOverlayWidget::mouseMoved, this, &ClickableVideoWidget::mouseMoved);
    connect(m_overlayWidget, &VideoOverlayWidget::mouseReleased, this, &ClickableVideoWidget::mouseReleased);
}

void ClickableVideoWidget::setLine(const QPoint &start, const QPoint &end)
{
    m_overlayWidget->setLine(start, end);
}

void ClickableVideoWidget::clearLine()
{
    m_overlayWidget->clearLine();
}

void ClickableVideoWidget::resizeEvent(QResizeEvent *event)
{
    QVideoWidget::resizeEvent(event);
    // 오버레이 위젯의 크기를 비디오 위젯과 동일하게 조정
    m_overlayWidget->resize(event->size());
}
