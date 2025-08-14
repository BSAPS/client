#include "customtitlebar.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QMouseEvent>
#include <QApplication>

CustomTitleBar::CustomTitleBar(QWidget *parent)
    : QWidget(parent) {
    setupUi();
}

void CustomTitleBar::setupUi() {
    setFixedHeight(30);
    setStyleSheet("background-color: #2c2c2c;");

    titleLayout = new QHBoxLayout(this);
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->setSpacing(0);

    titleLabel = new QLabel("Custom Window", this);
    titleLabel->setStyleSheet("color: white; font-weight: bold; padding-left: 10px;");
    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();

    minimizeButton = new QPushButton("-", this);
    closeButton = new QPushButton("X", this);

    minimizeButton->setFixedSize(30, 30);
    closeButton->setFixedSize(30, 30);

    minimizeButton->setStyleSheet("QPushButton { border: none; background-color: transparent; color: white; } QPushButton:hover { background-color: #555555; }");
    closeButton->setStyleSheet("QPushButton { border: none; background-color: transparent; color: white; } QPushButton:hover { background-color: #e51400; }");

    titleLayout->addWidget(minimizeButton);
    titleLayout->addWidget(closeButton);

    connect(minimizeButton, &QPushButton::clicked, this, [this]() { emit minimizeClicked(); });
    connect(closeButton, &QPushButton::clicked, this, [this]() { emit closeClicked(); });
}

void CustomTitleBar::setTitle(const QString &title) {
    titleLabel->setText(title);
}

void CustomTitleBar::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        mousePressPosition = event->globalPosition().toPoint() - parentWidget()->frameGeometry().topLeft();
        event->accept();
    }
}

void CustomTitleBar::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() & Qt::LeftButton) {
        parentWidget()->move(event->globalPosition().toPoint() - mousePressPosition);
        event->accept();
    }
}
