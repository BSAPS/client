#include "custommessagebox.h" // 헤더 파일 포함
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QApplication> // main 함수가 다른 곳에 있다면 필요 없을 수 있습니다.

CustomMessageBox::CustomMessageBox(QWidget *parent, const QString &title, const QString &message)
    : QDialog(parent) {
    // 윈도우 플래그 설정:
    // Qt::FramelessWindowHint - 프레임(테두리, 상단바)을 없앰
    // Qt::Popup - 팝업 윈도우처럼 동작 (옵션)
    // Qt::WindowStaysOnTopHint - 항상 위에 있도록 (옵션)
    setWindowFlags(Qt::FramelessWindowHint | Qt::Popup); // | Qt::WindowStaysOnTopHint);

    // 레이아웃 초기화 및 설정
    mainLayout = new QVBoxLayout(this);

    // 메시지 라벨 초기화 및 설정
    messageLabel = new QLabel(message);
    messageLabel->setAlignment(Qt::AlignCenter); // 텍스트 중앙 정렬
    mainLayout->addWidget(messageLabel);

    // 확인 버튼 초기화 및 설정
    okButton = new QPushButton("확인");
    mainLayout->addWidget(okButton);

    // 확인 버튼 클릭 시 다이얼로그 닫기
    connect(okButton, &QPushButton::clicked, this, &CustomMessageBox::accept);

    // 다이얼로그 크기 조절
    adjustSize();

    // 타이틀은 QDialog의 setWindowTitle 함수를 통해 설정할 수 있지만,
    // Qt::FramelessWindowHint 플래그가 설정되어 있으면 타이틀바가 보이지 않으므로
    // 시각적으로 큰 의미는 없습니다. 하지만 내부적으로는 설정됩니다.
    setWindowTitle(title);
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
