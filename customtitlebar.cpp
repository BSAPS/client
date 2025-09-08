#include "CustomTitleBar.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QMouseEvent>
#include <QApplication>

/**
 * @brief CustomTitleBar 생성자
 * @details 커스텀 타이틀바를 생성하고 UI를 초기화합니다.
 * @param parent 부모 위젯
 */
CustomTitleBar::CustomTitleBar(QWidget *parent)
    : QWidget(parent) {
    setupUi();
}

/**
 * @brief CustomTitleBar 소멸자
 * @details 임시 파일 메모리 해제
 */
CustomTitleBar::~CustomTitleBar(){
    if (m_tempFile) {
        delete m_tempFile;
        m_tempFile = nullptr;
    }
}

/**
 * @brief UI 설정 함수
 * @details 타이틀바의 UI 요소들을 초기화합니다.
 */
void CustomTitleBar::setupUi() {
    setFixedHeight(30);
    setStyleSheet("background-color: #474B5C;"); // 원래 배경색 : #272f42

    titleLayout = new QHBoxLayout(this);
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->setSpacing(0);

    titleLabel = new QLabel("Custom Window", this);
    titleLabel->setStyleSheet("color: white; font-weight: bold; padding-left: 10px;");
    titleLayout->addWidget(titleLabel);

    toolTipButton = new QPushButton("?", this);
    minimizeButton = new QPushButton("-", this);
    closeButton = new QPushButton("X", this);

    toolTipButton->setFixedSize(30, 30);
    minimizeButton->setFixedSize(30, 30);
    closeButton->setFixedSize(30, 30);

    toolTipButton->setStyleSheet("QPushButton { border: none; color: white; } QPushButton:hover { background-color: #555555; }");
    minimizeButton->setStyleSheet("QPushButton { border: none; color: white; } QPushButton:hover { background-color: #555555; }");
    closeButton->setStyleSheet("QPushButton { border: none; color: white; } QPushButton:hover { background-color: #e51400; }");

    titleLayout->addWidget(toolTipButton);
    titleLayout->addWidget(minimizeButton);
    titleLayout->addWidget(closeButton);

    connect(toolTipButton, &QPushButton::clicked, this, &CustomTitleBar::onToolTipClicked);
    connect(minimizeButton, &QPushButton::clicked, this, [this]() { emit minimizeClicked(); });
    connect(closeButton, &QPushButton::clicked, this, [this]() { emit closeClicked(); });
}

/**
 * @brief 타이틀 텍스트 설정
 * @param title 타이틀 문자열
 */
void CustomTitleBar::setTitle(const QString &title) {
    titleLabel->setText(title);
}

/**
 * @brief 마우스 클릭 이벤트 처리
 * @details 창 이동을 위한 드래그 시작 위치를 저장합니다.
 * @param event 마우스 이벤트
 */
void CustomTitleBar::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        mousePressPosition = event->globalPosition().toPoint() - window()->frameGeometry().topLeft();
        event->accept();
    }
}

/**
 * @brief 마우스 이동 이벤트 처리
 * @details 창을 드래그하여 이동합니다.
 * @param event 마우스 이벤트
 */
void CustomTitleBar::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() & Qt::LeftButton) {
        window()->move(event->globalPosition().toPoint() - mousePressPosition);
        event->accept();
    }
}

/**
 * @brief 툴팁 버튼 클릭 슬롯
 * @details 리소스 파일을 임시 파일로 복사 후 브라우저로 엽니다.
 */
void CustomTitleBar::onToolTipClicked(){
    if (m_tempFile) {
        delete m_tempFile;
        m_tempFile = nullptr;
    }

    // 1. 임시 파일 생성 (멤버 변수에 할당)
    m_tempFile = new QTemporaryFile(QDir::temp().filePath("temp_XXXXXX.html"));

    // 2. 리소스 파일 내용을 임시 파일로 복사
    QFile resourceFile(":/docs/index.html");
    if (!resourceFile.open(QFile::ReadOnly)) {
        QMessageBox::warning(nullptr, "오류", "리소스를 열 수 없습니다.");
        return;
    }
    if (!m_tempFile->open()) {
        QMessageBox::warning(nullptr, "오류", "임시 파일을 생성할 수 없습니다.");
        return;
    }
    m_tempFile->write(resourceFile.readAll());
    m_tempFile->close();
    resourceFile.close();

    // 3. 임시 파일의 경로를 QUrl::fromLocalFile()로 변환
    QUrl fileUrl = QUrl::fromLocalFile(m_tempFile->fileName());

    // 4. 웹 브라우저로 열기
    QDesktopServices::openUrl(fileUrl);
}
