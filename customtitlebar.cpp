#include "CustomTitleBar.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QMouseEvent>
#include <QApplication>

CustomTitleBar::CustomTitleBar(QWidget *parent)
    : QWidget(parent) {
    setupUi();
}

CustomTitleBar::~CustomTitleBar(){
    if (m_tempFile) {
        delete m_tempFile;
        m_tempFile = nullptr;
    }
}

void CustomTitleBar::setupUi() {
    setFixedHeight(30);
    setStyleSheet("background-color: #474B5C;"); // 원래 배경색 : #272f42

    titleLayout = new QHBoxLayout(this);
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->setSpacing(0);

    titleLabel = new QLabel("Custom Window", this);
    titleLabel->setStyleSheet("color: white; font-weight: bold; padding-left: 10px;");
    titleLayout->addWidget(titleLabel);
    // titleLayout->addStretch(); // 이 코드를 사용하면 글자랑 버튼 사이에 늘리기만 하고 스타일시트가 적용이 안됨

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

void CustomTitleBar::setTitle(const QString &title) {
    titleLabel->setText(title);
}

void CustomTitleBar::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        mousePressPosition = event->globalPosition().toPoint() - window()->frameGeometry().topLeft();
        event->accept();
    }
}

void CustomTitleBar::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() & Qt::LeftButton) {
        window()->move(event->globalPosition().toPoint() - mousePressPosition);
        event->accept();
    }
}

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
