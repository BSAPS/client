#include "custommessagebox.h" // 헤더 파일 포함
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QScreen>
#include <QApplication> // main 함수가 다른 곳에 있다면 필요 없을 수 있습니다.
#include <QGraphicsDropShadowEffect>
#include <QMouseEvent>

CustomMessageBox::CustomMessageBox(QWidget *parent, const QString &title, const QString &message)
    : QDialog(parent), m_dragging(false)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::CustomizeWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    QVBoxLayout *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(20, 20, 20, 20); // 그림자 공간 확보

    QFrame *popup = new QFrame(this);
    popup->setObjectName("popupFrame");
    popup->setStyleSheet(R"(
        QFrame#popupFrame {
            background-color: #3c405c;
            border-radius: 12px;
        })");

    QVBoxLayout *mainLayout = new QVBoxLayout(popup);
    mainLayout->setContentsMargins(24, 24, 24, 24);

    QLabel *messageLabel = new QLabel(message);
    messageLabel->setStyleSheet("color: white; font-size: 14px;");
    messageLabel->setWordWrap(true);
    //messageLabel->setMinimumWidth(300);
    messageLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(messageLabel);

    QPushButton *okButton = new QPushButton("확인");
    okButton->setFixedSize(100, 36);
    okButton->setStyleSheet(R"(
        QPushButton {
            background-color: #5d6075;
            color: white;
            border: none;
            border-radius: 6px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #6f728a;
        }
        QPushButton:pressed {
            background-color: #4b4e65;
        })");
    // 메시지 아래 간격 추가 (예: 16px)
    mainLayout->addSpacing(16);
    mainLayout->addWidget(okButton, 0, Qt::AlignCenter);

    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);

    // 그림자 효과
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(popup);
    shadow->setBlurRadius(24);
    shadow->setOffset(0, 6);
    shadow->setColor(QColor(0, 0, 0, 160));
    popup->setGraphicsEffect(shadow);

    outerLayout->addWidget(popup);

    // 중앙 배치
    QRect screen = QGuiApplication::primaryScreen()->availableGeometry();
    move(screen.center() - QPoint(width() / 2, height() / 2));
    adjustSize();
}


void CustomMessageBox::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragPos = event->globalPosition().toPoint() - frameGeometry().topLeft();
    }
}

void CustomMessageBox::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging && event->buttons() & Qt::LeftButton) {
        move(event->globalPosition().toPoint() - m_dragPos);
    }
}

void CustomMessageBox::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    m_dragging = false;
}


//--- 너비와 높이를 정수 값으로 설정하는 함수
void CustomMessageBox::setFixedSize(int width, int height) {
    QDialog::setFixedSize(width, height);
}

//--- QSize 객체로 너비와 높이를 설정하는 함수
void CustomMessageBox::setFixedSize(const QSize &size) {
    QDialog::setFixedSize(size);
}

// 필요하다면 여기에 CustomMessageBox::~CustomMessageBox() 소멸자를 구현합니다.
// CustomMessageBox::~CustomMessageBox() {
//     // 동적으로 할당된 위젯들은 부모-자식 관계에 의해 자동으로 삭제되므로
//     // 특별히 해제할 것이 없다면 비워두거나 구현하지 않아도 됩니다.
// }
