#include "MainWindow_simple.h"
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_centralWidget(nullptr)
    , m_label(nullptr)
    , m_button(nullptr)
{
    setupUI();
    setWindowTitle("CCTV Monitoring - Simple Test");
    resize(400, 300);
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUI()
{
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);
    
    QVBoxLayout *layout = new QVBoxLayout(m_centralWidget);
    
    m_label = new QLabel("CCTV 모니터링 시스템 테스트");
    m_label->setAlignment(Qt::AlignCenter);
    m_label->setStyleSheet("font-size: 18px; color: #333; padding: 20px;");
    
    m_button = new QPushButton("테스트 버튼");
    m_button->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; padding: 10px; border: none; border-radius: 5px; font-size: 14px; }");
    
    connect(m_button, &QPushButton::clicked, this, &MainWindow::onButtonClicked);
    
    layout->addWidget(m_label);
    layout->addWidget(m_button);
    layout->addStretch();
}

void MainWindow::onButtonClicked()
{
    QMessageBox::information(this, "테스트", "Qt 빌드가 정상적으로 작동합니다!");
}
