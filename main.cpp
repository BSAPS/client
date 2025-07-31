#include <QApplication>
#include <QStyleFactory>
#include <QDir>
#include <QDebug>
#include <QFontDatabase>
#include "LoginWindow.h"
#include "MainWindow.h"
#include "TcpCommunicator.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);


    // 애플리케이션 정보 설정
    app.setApplicationName("CCTV Monitoring System");
    app.setApplicationVersion("2.0");
    app.setOrganizationName("CCTV Solutions");

    // 스타일 설정
    app.setStyle(QStyleFactory::create("Fusion"));

    //폰트 적용
    int fontId = QFontDatabase::addApplicationFont(":/font/05HanwhaGothicR.ttf");
    QString family = QFontDatabase::applicationFontFamilies(fontId).at(0);
    QFont customFont(family);
    customFont.setPointSize(10);
    // customFont.setBold(true);
    app.setFont(customFont);

    // 다크 테마 스타일 적용
    app.setStyle(QStyleFactory::create("Fusion"));
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(41, 45, 65));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);
    app.setPalette(darkPalette);

    // 공유 TcpCommunicator 생성
    TcpCommunicator *sharedTcpCommunicator = new TcpCommunicator();

    // 로그인 창 생성 및 표시
    LoginWindow loginWindow;

    loginWindow.setWindowFlags(Qt::Window | Qt::FramelessWindowHint);

    
    // LoginWindow에 공유 TcpCommunicator 설정
    loginWindow.setTcpCommunicator(sharedTcpCommunicator);

    // 로그인 성공 시 메인 창으로 전환
    QObject::connect(&loginWindow, &LoginWindow::loginSuccessful, [&]() {
        qDebug() << "로그인 성공 - 메인 창 표시";

        MainWindow *mainWindow = new MainWindow();

        mainWindow->setWindowFlags(Qt::Window | Qt::FramelessWindowHint);

        // MainWindow에도 동일한 TcpCommunicator 설정
        mainWindow->setTcpCommunicator(sharedTcpCommunicator);
        

        mainWindow->show();

        qDebug() << "메인 창이 표시되었습니다.";
    });

    // 로그인 창 표시
    if (loginWindow.exec() == QDialog::Accepted) {
        qDebug() << "로그인 다이얼로그가 성공적으로 완료되었습니다.";
        return app.exec();
    } else {
        qDebug() << "로그인이 취소되었습니다.";
        delete sharedTcpCommunicator; // 로그인 실패 시 정리
        return 0;
    }
}
