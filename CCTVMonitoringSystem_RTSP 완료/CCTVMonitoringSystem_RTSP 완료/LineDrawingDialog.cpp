#include "LineDrawingDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QApplication>
#include <QScreen>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QDebug>

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

// ClickableVideoWidget 구현
ClickableVideoWidget::ClickableVideoWidget(QWidget *parent)
    : QVideoWidget(parent)
    , m_hasLine(false)
{
    setMouseTracking(true);
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setAttribute(Qt::WA_NoSystemBackground, true);
}

void ClickableVideoWidget::setLine(const QPoint &start, const QPoint &end)
{
    m_lineStart = start;
    m_lineEnd = end;
    m_hasLine = true;
    update();
}

void ClickableVideoWidget::clearLine()
{
    m_hasLine = false;
    update();
}

void ClickableVideoWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit mousePressed(event->pos());
    }
    QVideoWidget::mousePressEvent(event);
}

void ClickableVideoWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        emit mouseMoved(event->pos());
    }
    QVideoWidget::mouseMoveEvent(event);
}

void ClickableVideoWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit mouseReleased(event->pos());
    }
    QVideoWidget::mouseReleaseEvent(event);
}

void ClickableVideoWidget::paintEvent(QPaintEvent *event)
{
    QVideoWidget::paintEvent(event);

    if (m_hasLine) {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // 기준선 그리기
        QPen pen(Qt::red, 3, Qt::SolidLine);
        painter.setPen(pen);
        painter.drawLine(m_lineStart, m_lineEnd);

        // 시작점과 끝점에 원 그리기
        painter.setBrush(Qt::red);
        painter.drawEllipse(m_lineStart, 5, 5);
        painter.drawEllipse(m_lineEnd, 5, 5);
    }
}
