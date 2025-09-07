#include <QApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <QStyleFactory>
#include <QDir>
#include <QDebug>
#include <QFontDatabase>
#include "LoginWindow.h"
#include "MainWindow.h"
#include "TcpCommunicator.h"
#include "custommessagebox.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    //폰트 적용
    int fontId = QFontDatabase::addApplicationFont(":/font/05HanwhaGothicR.ttf");
    QString family = QFontDatabase::applicationFontFamilies(fontId).at(0);
    QFont customFont(family);
    customFont.setPointSize(10);
    app.setFont(customFont);

    // 고유한 서버 이름 설정 (한 PC에 하나의 프로그램만 실행 가능하게 하기 위해)
    const QString serverName = "com.bsaps.cctvmonitoring-v1.0.lock";
    QLocalSocket socket;
    socket.connectToServer(serverName, QIODevice::WriteOnly);
    // 이미 서버가 실행 중인지 먼저 확인
    if (socket.waitForConnected(500)) {
        CustomMessageBox msgBox(nullptr, "연결 오류", "프로그램이 이미 실행 중입니다.");
        msgBox.exec();
        return 0; // 즉시 종료
    }

    QLocalServer localServer;

    // 이전 실행에서 남은 서버 소켓 파일이 있다면 제거
    QLocalServer::removeServer(serverName);

    // 서버 시작 시도
    if (!localServer.listen(serverName)) {
        // 서버를 시작하지 못했다면 (다른 인스턴스 존재)
        CustomMessageBox msgBox(nullptr, "연결 오류", "프로그램이 이미 실행 중입니다.");
        msgBox.exec();
        return 0;
    }

    // 애플리케이션 아이콘 설정
    QIcon app_icon(":/icons/CCTV.png");
    app.setWindowIcon(app_icon);


    // 애플리케이션 정보 설정
    app.setApplicationName("CCTV Monitoring System");
    app.setApplicationVersion("2.0");
    app.setOrganizationName("CCTV Solutions");

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
    // LoginWindow에 공유 TcpCommunicator 설정
    loginWindow.setTcpCommunicator(sharedTcpCommunicator);

    // 로그인 창 표시
    if (loginWindow.exec() == QDialog::Accepted) {
        qDebug() << "로그인 다이얼로그가 성공적으로 완료되었습니다.";
        MainWindow *mainWindow = new MainWindow();
        mainWindow->setTcpCommunicator(sharedTcpCommunicator);
        mainWindow->show();
        return app.exec();
    } else {
        qDebug() << "로그인이 취소되었습니다.";
        delete sharedTcpCommunicator; // 로그인 실패 시 정리
        return 0;
    }
}
