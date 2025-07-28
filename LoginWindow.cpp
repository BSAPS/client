#include "LoginWindow.h"
#include "ui_LoginWindow.h"
#include "MainWindow.h"
#include "TcpCommunicator.h"
#include "EnvConfig.h"
#include <QApplication>
#include <QMessageBox>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QTimer>
#include <QPixmap>
#include <QBuffer>
#include <QByteArray>
#include <QRegularExpression>


#include <QtSvg/QSvgRenderer>
#include <QPainter>
#include <QLabel>

LoginWindow::LoginWindow(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui_LoginWindow)
    , m_tcpCommunicator(nullptr)
    , m_passwordErrorLabel(nullptr)
    , m_closeSignUpButton(nullptr)
    , m_closeOtpSignUpButton(nullptr)
    , m_connectionStatusLabel(nullptr)
    , m_connectionTimer(nullptr)
{
    qDebug() << "[LoginWindow] 생성자 시작";

    // .env 파일 로드
    EnvConfig::loadFromFile();
    
    // 네트워크 설정 초기화
    m_tcpHost = EnvConfig::getValue("TCP_HOST", "192.168.0.81");
    m_tcpPort = EnvConfig::getValue("TCP_PORT", "8080").toUInt();
    
    qDebug() << "[LoginWindow] .env 설정 로드 - TCP_HOST:" << m_tcpHost << "TCP_PORT:" << m_tcpPort;

    ui->setupUi(this);

    // 초기화 메서드 호출
    setupPasswordFields();
    setupPasswordErrorLabel();
    setupCloseButtons();
    setupConnectionStatusLabel();

    qDebug() << "[LoginWindow] UI 설정 완료";

    // TCP 통신 설정
    setupTcpCommunication();

    qDebug() << "[LoginWindow] TCP 통신 설정 완료";

    // 시그널 연결
    connectSignals();

    qDebug() << "[LoginWindow] 시그널 연결 완료";

    // 초기 페이지 설정
    ui->stackedWidget->setCurrentIndex(1); // 회원가입 영역으로 바로 이동
    ui->OTPLoginWidget_3->setCurrentIndex(1); // OTPSignUpPage로 바로 이동

    // 다이얼로그 설정
    setModal(true);
    setWindowTitle("CCTV 모니터링 시스템 - 로그인");

    // 연결 상태 확인 타이머 설정
    setupConnectionTimer();

    qDebug() << "[LoginWindow] 생성자 완료";

    // ================== [테스트용 QR코드/복구코드 표시 - 실제 배포 전 반드시 삭제] ==================
    QJsonObject response;
    response["qr_code_svg"] = R"(<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" viewBox=\"0 0 57.000000 57.000000\" stroke=\"none\">\n\t<rect width=\"100%\" height=\"100%\" fill=\"#FFFFFF\"/>\n\t<g transform=\"translate(0.000000 0.000000)\">\n\t\t\t<path d=\"M4,4h1v1h-1z M5,4h1v1h-1z M6,4h1v1h-1z M7,4h1v1h-1z M8,4h1v1h-1z M9,4h1v1h-1z M10,4h1v1h-1z M15,4h1v1h-1z M16,4h1v1h-1z M17,4h1v1h-1z M19,4h1v1h-1z M21,4h1v1h-1z M22,4h1v1h-1z M25,4h1v1h-1z M27,4h1v1h-1z M29,4h1v1h-1z M30,4h1v1h-1z M31,4h1v1h-1z M32,4h1v1h-1z M34,4h1v1h-1z M41,4h1v1h-1z M44,4h1v1h-1z M46,4h1v1h-1z M47,4h1v1h-1z M48,4h1v1h-1z M49,4h1v1h-1z M50,4h1v1h-1z M51,4h1v1h-1z M52,4h1v1h-1z M4,5h1v1h-1z M10,5h1v1h-1z M16,5h1v1h-1z M18,5h1v1h-1z M20,5h1v1h-1z M21,5h1v1h-1z M23,5h1v1h-1z M25,5h1v1h-1z M26,5h1v1h-1z M27,5h1v1h-1z M28,5h1v1h-1z M32,5h1v1h-1z M33,5h1v1h-1z M35,5h1v1h-1z M38,5h1v1h-1z M39,5h1v1h-1z M40,5h1v1h-1z M41,5h1v1h-1z M42,5h1v1h-1z M43,5h1v1h-1z M44,5h1v1h-1z M46,5h1v1h-1z M52,5h1v1h-1z M4,6h1v1h-1z M6,6h1v1h-1z M7,6h1v1h-1z M8,6h1v1h-1z M10,6h1v1h-1z M12,6h1v1h-1z M13,6h1v1h-1z M16,6h1v1h-1z M17,6h1v1h-1z M23,6h1v1h-1z M25,6h1v1h-1z M28,6h1v1h-1z M29,6h1v1h-1z M30,6h1v1h-1z M31,6h1v1h-1z M32,6h1v1h-1z M34,6h1v1h-1z M35,6h1v1h-1z M36,6h1v1h-1z M37,6h1v1h-1z M43,6h1v1h-1z M44,6h1v1h-1z M46,6h1v1h-1z M48,6h1v1h-1z M49,6h1v1h-1z M50,6h1v1h-1z M52,6h1v1h-1z M4,7h1v1h-1z M6,7h1v1h-1z M7,7h1v1h-1z M8,7h1v1h-1z M10,7h1v1h-1z M12,7h1v1h-1z M13,7h1v1h-1z M14,7h1v1h-1z M15,7h1v1h-1z M16,7h1v1h-1z M17,7h1v1h-1z M18,7h1v1h-1z M26,7h1v1h-1z M27,7h1v1h-1z M29,7h1v1h-1z M30,7h1v1h-1z M34,7h1v1h-1z M38,7h1v1h-1z M39,7h1v1h-1z M40,7h1v1h-1z M41,7h1v1h-1z M43,7h1v1h-1z M46,7h1v1h-1z M48,7h1v1h-1z M49,7h1v1h-1z M50,7h1v1h-1z M52,7h1v1h-1z M4,8h1v1h-1z M6,8h1v1h-1z M7,8h1v1h-1z M8,8h1v1h-1z M10,8h1v1h-1z M12,8h1v1h-1z M15,8h1v1h-1z M16,8h1v1h-1z M17,8h1v1h-1z M21,8h1v1h-1z M23,8h1v1h-1z M24,8h1v1h-1z M25,8h1v1h-1z M26,8h1v1h-1z M27,8h1v1h-1z M28,8h1v1h-1z M29,8h1v1h-1z M30,8h1v1h-1z M31,8h1v1h-1z M33,8h1v1h-1z M34,8h1v1h-1z M35,8h1v1h-1z M40,8h1v1h-1z M41,8h1v1h-1z M46,8h1v1h-1z M48,8h1v1h-1z M49,8h1v1h-1z M50,8h1v1h-1z M52,8h1v1h-1z M4,9h1v1h-1z M10,9h1v1h-1z M12,9h1v1h-1z M13,9h1v1h-1z M14,9h1v1h-1z M17,9h1v1h-1z M18,9h1v1h-1z M19,9h1v1h-1z M24,9h1v1h-1z M26,9h1v1h-1z M30,9h1v1h-1z M34,9h1v1h-1z M35,9h1v1h-1z M37,9h1v1h-1z M39,9h1v1h-1z M40,9h1v1h-1z M41,9h1v1h-1z M42,9h1v1h-1z M46,9h1v1h-1z M52,9h1v1h-1z M4,10h1v1h-1z M5,10h1v1h-1z M6,10h1v1h-1z M7,10h1v1h-1z M8,10h1v1h-1z M9,10h1v1h-1z M10,10h1v1h-1z M12,10h1v1h-1z M14,10h1v1h-1z M16,10h1v1h-1z M18,10h1v1h-1z M20,10h1v1h-1z M22,10h1v1h-1z M24,10h1v1h-1z M26,10h1v1h-1z M28,10h1v1h-1z M30,10h1v1h-1z M32,10h1v1h-1z M34,10h1v1h-1z M36,10h1v1h-1z M38,10h1v1h-1z M40,10h1v1h-1z M42,10h1v1h-1z M44,10h1v1h-1z M46,10h1v1h-1z M47,10h1v1h-1z M48,10h1v1h-1z M49,10h1v1h-1z M50,10h1v1h-1z M51,10h1v1h-1z M52,10h1v1h-1z M12,11h1v1h-1z M13,11h1v1h-1z M15,11h1v1h-1z M17,11h1v1h-1z M19,11h1v1h-1z M20,11h1v1h-1z M22,11h1v1h-1z M23,11h1v1h-1z M24,11h1v1h-1z M25,11h1v1h-1z M26,11h1v1h-1z M30,11h1v1h-1z M31,11h1v1h-1z M33,11h1v1h-1z M34,11h1v1h-1z M39,11h1v1h-1z M41,11h1v1h-1z M42,11h1v1h-1z M44,11h1v1h-1z M4,12h1v1h-1z M6,12h1v1h-1z M7,12h1v1h-1z M8,12h1v1h-1z M9,12h1v1h-1z M10,12h1v1h-1z M13,12h1v1h-1z M14,12h1v1h-1z M17,12h1v1h-1z M18,12h1v1h-1z M19,12h1v1h-1z M21,12h1v1h-1z M24,12h1v1h-1z M25,12h1v1h-1z M26,12h1v1h-1z M27,12h1v1h-1z M28,12h1v1h-1z M29,12h1v1h-1z M30,12h1v1h-1z M33,12h1v1h-1z M35,12h1v1h-1z M37,12h1v1h-1z M39,12h1v1h-1z M40,12h1v1h-1z M41,12h1v1h-1z M42,12h1v1h-1z M43,12h1v1h-1z M44,12h1v1h-1z M46,12h1v1h-1z M47,12h1v1h-1z M48,12h1v1h-1z M49,12h1v1h-1z M50,12h1v1h-1z M5,13h1v1h-1z M7,13h1v1h-1z M12,13h1v1h-1z M13,13h1v1h-1z M16,13h1v1h-1z M19,13h1v1h-1z M22,13h1v1h-1z M23,13h1v1h-1z M26,13h1v1h-1z M29,13h1v1h-1z M32,13h1v1h-1z M34,13h1v1h-1z M35,13h1v1h-1z M36,13h1v1h-1z M37,13h1v1h-1z M39,13h1v1h-1z M40,13h1v1h-1z M41,13h1v1h-1z M46,13h1v1h-1z M47,13h1v1h-1z M5,14h1v1h-1z M6,14h1v1h-1z M10,14h1v1h-1z M13,14h1v1h-1z M17,14h1v1h-1z M19,14h1v1h-1z M20,14h1v1h-1z M22,14h1v1h-1z M25,14h1v1h-1z M27,14h1v1h-1z M29,14h1v1h-1z M30,14h1v1h-1z M33,14h1v1h-1z M35,14h1v1h-1z M38,14h1v1h-1z M39,14h1v1h-1z M40,14h1v1h-1z M41,14h1v1h-1z M42,14h1v1h-1z M44,14h1v1h-1z M51,14h1v1h-1z M52,14h1v1h-1z M6,15h1v1h-1z M7,15h1v1h-1z M9,15h1v1h-1z M11,15h1v1h-1z M16,15h1v1h-1z M17,15h1v1h-1z M18,15h1v1h-1z M20,15h1v1h-1z M21,15h1v1h-1z M23,15h1v1h-1z M29,15h1v1h-1z M30,15h1v1h-1z M31,15h1v1h-1z M32,15h1v1h-1z M34,15h1v1h-1z M35,15h1v1h-1z M36,15h1v1h-1z M38,15h1v1h-1z M40,15h1v1h-1z M45,15h1v1h-1z M46,15h1v1h-1z M47,15h1v1h-1z M52,15h1v1h-1z M5,16h1v1h-1z M7,16h1v1h-1z M10,16h1v1h-1z M11,16h1v1h-1z M15,16h1v1h-1z M18,16h1v1h-1z M19,16h1v1h-1z M20,16h1v1h-1z M21,16h1v1h-1z M23,16h1v1h-1z M25,16h1v1h-1z M26,16h1v1h-1z M27,16h1v1h-1z M28,16h1v1h-1z M29,16h1v1h-1z M30,16h1v1h-1z M38,16h1v1h-1z M39,16h1v1h-1z M40,16h1v1h-1z M44,16h1v1h-1z M46,16h1v1h-1z M47,16h1v1h-1z M50,16h1v1h-1z M51,16h1v1h-1z M52,16h1v1h-1z M5,17h1v1h-1z M6,17h1v1h-1z M7,17h1v1h-1z M9,17h1v1h-1z M14,17h1v1h-1z M17,17h1v1h-1z M18,17h1v1h-1z M21,17h1v1h-1z M22,17h1v1h-1z M25,17h1v1h-1z M27,17h1v1h-1z M28,17h1v1h-1z M31,17h1v1h-1z M34,17h1v1h-1z M36,17h1v1h-1z M39,17h1v1h-1z M41,17h1v1h-1z M43,17h1v1h-1z M45,17h1v1h-1z M46,17h1v1h-1z M47,17h1v1h-1z M48,17h1v1h-1z M50,17h1v1h-1z M51,17h1v1h-1z M5,18h1v1h-1z M6,18h1v1h-1z M8,18h1v1h-1z M10,18h1v1h-1z M15,18h1v1h-1z M17,18h1v1h-1z M19,18h1v1h-1z M20,18h1v1h-1z M22,18h1v1h-1z M23,18h1v1h-1z M28,18h1v1h-1z M29,18h1v1h-1z M30,18h1v1h-1z M33,18h1v1h-1z M38,18h1v1h-1z M39,18h1v1h-1z M43,18h1v1h-1z M44,18h1v1h-1z M51,18h1v1h-1z M52,18h1v1h-1z M4,19h1v1h-1z M5,19h1v1h-1z M8,19h1v1h-1z M9,19h1v1h-1z M11,19h1v1h-1z M12,19h1v1h-1z M15,19h1v1h-1z M17,19h1v1h-1z M21,19h1v1h-1z M22,19h1v1h-1z M25,19h1v1h-1z M27,19h1v1h-1z M29,19h1v1h-1z M33,19h1v1h-1z M36,19h1v1h-1z M37,19h1v1h-1z M42,19h1v1h-1z M43,19h1v1h-1z M45,19h1v1h-1z M46,19h1v1h-1z M48,19h1v1h-1z M51,19h1v1h-1z M4,20h1v1h-1z M7,20h1v1h-1z M8,20h1v1h-1z M9,20h1v1h-1z M10,20h1v1h-1z M11,20h1v1h-1z M12,20h1v1h-1z M17,20h1v1h-1z M18,20h1v1h-1z M19,20h1v1h-1z M22,20h1v1h-1z M23,20h1v1h-1z M24,20h1v1h-1z M28,20h1v1h-1z M29,20h1v1h-1z M31,20h1v1h-1z M33,20h1v1h-1z M34,20h1v1h-1z M35,20h1v1h-1z M37,20h1v1h-1z M40,20h1v1h-1z M41,20h1v1h-1z M44,20h1v1h-1z M46,20h1v1h-1z M47,20h1v1h-1z M51,20h1v1h-1z M8,21h1v1h-1z M9,21h1v1h-1z M15,21h1v1h-1z M16,21h1v1h-1z M17,21h1v1h-1z M19,21h1v1h-1z M20,21h1v1h-1z M23,21h1v1h-1z M27,21h1v1h-1z M28,21h1v1h-1z M29,21h1v1h-1z M34,21h1v1h-1z M35,21h1v1h-1z M36,21h1v1h-1z M41,21h1v1h-1z M43,21h1v1h-1z M45,21h1v1h-1z M46,21h1v1h-1z M47,21h1v1h-1z M51,21h1v1h-1z M4,22h1v1h-1z M7,22h1v1h-1z M10,22h1v1h-1z M12,22h1v1h-1z M13,22h1v1h-1z M18,22h1v1h-1z M20,22h1v1h-1z M23,22h1v1h-1z M30,22h1v1h-1z M31,22h1v1h-1z M33,22h1v1h-1z M35,22h1v1h-1z M36,22h1v1h-1z M37,22h1v1h-1z M38,22h1v1h-1z M39,22h1v1h-1z M40,22h1v1h-1z M41,22h1v1h-1z M42,22h1v1h-1z M43,22h1v1h-1z M44,22h1v1h-1z M49,22h1v1h-1z M52,22h1v1h-1z M6,23h1v1h-1z M11,23h1v1h-1z M12,23h1v1h-1z M14,23h1v1h-1z M15,23h1v1h-1z M16,23h1v1h-1z M17,23h1v1h-1z M20,23h1v1h-1z M24,23h1v1h-1z M25,23h1v1h-1z M32,23h1v1h-1z M33,23h1v1h-1z M36,23h1v1h-1z M37,23h1v1h-1z M39,23h1v1h-1z M41,23h1v1h-1z M44,23h1v1h-1z M45,23h1v1h-1z M46,23h1v1h-1z M47,23h1v1h-1z M48,23h1v1h-1z M51,23h1v1h-1z M52,23h1v1h-1z M4,24h1v1h-1z M5,24h1v1h-1z M7,24h1v1h-1z M8,24h1v1h-1z M9,24h1v1h-1z M10,24h1v1h-1z M14,24h1v1h-1z M17,24h1v1h-1z M18,24h1v1h-1z M19,24h1v1h-1z M21,24h1v1h-1z M22,24h1v1h-1z M23,24h1v1h-1z M24,24h1v1h-1z M26,24h1v1h-1z M27,24h1v1h-1z M28,24h1v1h-1z M29,24h1v1h-1z M33,24h1v1h-1z M34,24h1v1h-1z M38,24h1v1h-1z M39,24h1v1h-1z M40,24h1v1h-1z M43,24h1v1h-1z M44,24h1v1h-1z M46,24h1v1h-1z M47,24h1v1h-1z M50,24h1v1h-1z M51,24h1v1h-1z M6,25h1v1h-1z M7,25h1v1h-1z M9,25h1v1h-1z M11,25h1v1h-1z M14,25h1v1h-1z M15,25h1v1h-1z M16,25h1v1h-1z M20,25h1v1h-1z M22,25h1v1h-1z M23,25h1v1h-1z M32,25h1v1h-1z M33,25h1v1h-1z M34,25h1v1h-1z M35,25h1v1h-1z M36,25h1v1h-1z M39,25h1v1h-1z M40,25h1v1h-1z M41,25h1v1h-1z M43,25h1v1h-1z M45,25h1v1h-1z M46,25h1v1h-1z M47,25h1v1h-1z M4,26h1v1h-1z M8,26h1v1h-1z M9,26h1v1h-1z M10,26h1v1h-1z M11,26h1v1h-1z M12,26h1v1h-1z M14,26h1v1h-1z M16,26h1v1h-1z M17,26h1v1h-1z M20,26h1v1h-1z M23,26h1v1h-1z M24,26h1v1h-1z M25,26h1v1h-1z M26,26h1v1h-1z M27,26h1v1h-1z M28,26h1v1h-1z M29,26h1v1h-1z M30,26h1v1h-1z M31,26h1v1h-1z M35,26h1v1h-1z M42,26h1v1h-1z M44,26h1v1h-1z M45,26h1v1h-1z M46,26h1v1h-1z M47,26h1v1h-1z M48,26h1v1h-1z M51,26h1v1h-1z M52,26h1v1h-1z M4,27h1v1h-1z M7,27h1v1h-1z M8,27h1v1h-1z M12,27h1v1h-1z M13,27h1v1h-1z M15,27h1v1h-1z M17,27h1v1h-1z M21,27h1v1h-1z M25,27h1v1h-1z M26,27h1v1h-1z M30,27h1v1h-1z M33,27h1v1h-1z M34,27h1v1h-1z M35,27h1v1h-1z M38,27h1v1h-1z M44,27h1v1h-1z M48,27h1v1h-1z M51,27h1v1h-1z M52,27h1v1h-1z M4,28h1v1h-1z M5,28h1v1h-1z M6,28h1v1h-1z M8,28h1v1h-1z M10,28h1v1h-1z M12,28h1v1h-1z M14,28h1v1h-1z M15,28h1v1h-1z M18,28h1v1h-1z M19,28h1v1h-1z M20,28h1v1h-1z M22,28h1v1h-1z M23,28h1v1h-1z M24,28h1v1h-1z M25,28h1v1h-1z M26,28h1v1h-1z M28,28h1v1h-1z M30,28h1v1h-1z M35,28h1v1h-1z M39,28h1v1h-1z M40,28h1v1h-1z M41,28h1v1h-1z M42,28h1v1h-1z M44,28h1v1h-1z M46,28h1v1h-1z M48,28h1v1h-1z M49,28h1v1h-1z M50,28h1v1h-1z M51,28h1v1h-1z M52,28h1v1h-1z M4,29h1v1h-1z M5,29h1v1h-1z M8,29h1v1h-1z M12,29h1v1h-1z M14,29h1v1h-1z M15,29h1v1h-1z M18,29h1v1h-1z M20,29h1v1h-1z M21,29h1v1h-1z M23,29h1v1h-1z M25,29h1v1h-1z M26,29h1v1h-1z M30,29h1v1h-1z M31,29h1v1h-1z M32,29h1v1h-1z M33,29h1v1h-1z M34,29h1v1h-1z M36,29h1v1h-1z M37,29h1v1h-1z M41,29h1v1h-1z M44,29h1v1h-1z M48,29h1v1h-1z M49,29h1v1h-1z M51,29h1v1h-1z M5,30h1v1h-1z M7,30h1v1h-1z M8,30h1v1h-1z M9,30h1v1h-1z M10,30h1v1h-1z M11,30h1v1h-1z M12,30h1v1h-1z M14,30h1v1h-1z M16,30h1v1h-1z M20,30h1v1h-1z M22,30h1v1h-1z M23,30h1v1h-1z M24,30h1v1h-1z M26,30h1v1h-1z M27,30h1v1h-1z M28,30h1v1h-1z M29,30h1v1h-1z M30,30h1v1h-1z M31,30h1v1h-1z M32,30h1v1h-1z M34,30h1v1h-1z M38,30h1v1h-1z M40,30h1v1h-1z M42,30h1v1h-1z M43,30h1v1h-1z M44,30h1v1h-1z M45,30h1v1h-1z M46,30h1v1h-1z M47,30h1v1h-1z M48,30h1v1h-1z M49,30h1v1h-1z M50,30h1v1h-1z M51,30h1v1h-1z M52,30h1v1h-1z M4,31h1v1h-1z M5,31h1v1h-1z M7,31h1v1h-1z M8,31h1v1h-1z M9,31h1v1h-1z M14,31h1v1h-1z M16,31h1v1h-1z M22,31h1v1h-1z M25,31h1v1h-1z M27,31h1v1h-1z M29,31h1v1h-1z M32,31h1v1h-1z M34,31h1v1h-1z M35,31h1v1h-1z M36,31h1v1h-1z M37,31h1v1h-1z M41,31h1v1h-1z M42,31h1v1h-1z M44,31h1v1h-1z M45,31h1v1h-1z M46,31h1v1h-1z M47,31h1v1h-1z M49,31h1v1h-1z M51,31h1v1h-1z M4,32h1v1h-1z M5,32h1v1h-1z M10,32h1v1h-1z M12,32h1v1h-1z M15,32h1v1h-1z M16,32h1v1h-1z M17,32h1v1h-1z M26,32h1v1h-1z M27,32h1v1h-1z M28,32h1v1h-1z M34,32h1v1h-1z M37,32h1v1h-1z M38,32h1v1h-1z M40,32h1v1h-1z M41,32h1v1h-1z M43,32h1v1h-1z M44,32h1v1h-1z M45,32h1v1h-1z M46,32h1v1h-1z M47,32h1v1h-1z M48,32h1v1h-1z M50,32h1v1h-1z M52,32h1v1h-1z M14,33h1v1h-1z M15,33h1v1h-1z M18,33h1v1h-1z M19,33h1v1h-1z M20,33h1v1h-1z M21,33h1v1h-1z M23,33h1v1h-1z M24,33h1v1h-1z M25,33h1v1h-1z M26,33h1v1h-1z M28,33h1v1h-1z M30,33h1v1h-1z M33,33h1v1h-1z M34,33h1v1h-1z M35,33h1v1h-1z M40,33h1v1h-1z M41,33h1v1h-1z M43,33h1v1h-1z M49,33h1v1h-1z M6,34h1v1h-1z M8,34h1v1h-1z M9,34h1v1h-1z M10,34h1v1h-1z M12,34h1v1h-1z M14,34h1v1h-1z M15,34h1v1h-1z M18,34h1v1h-1z M19,34h1v1h-1z M21,34h1v1h-1z M24,34h1v1h-1z M26,34h1v1h-1z M27,34h1v1h-1z M28,34h1v1h-1z M29,34h1v1h-1z M31,34h1v1h-1z M32,34h1v1h-1z M33,34h1v1h-1z M34,34h1v1h-1z M35,34h1v1h-1z M37,34h1v1h-1z M38,34h1v1h-1z M40,34h1v1h-1z M42,34h1v1h-1z M44,34h1v1h-1z M45,34h1v1h-1z M46,34h1v1h-1z M47,34h1v1h-1z M50,34h1v1h-1z M51,34h1v1h-1z M52,34h1v1h-1z M6,35h1v1h-1z M12,35h1v1h-1z M15,35h1v1h-1z M16,35h1v1h-1z M17,35h1v1h-1z M20,35h1v1h-1z M22,35h1v1h-1z M23,35h1v1h-1z M24,35h1v1h-1z M28,35h1v1h-1z M30,35h1v1h-1z M31,35h1v1h-1z M34,35h1v1h-1z M36,35h1v1h-1z M39,35h1v1h-1z M41,35h1v1h-1z M51,35h1v1h-1z M6,36h1v1h-1z M7,36h1v1h-1z M9,36h1v1h-1z M10,36h1v1h-1z M11,36h1v1h-1z M12,36h1v1h-1z M15,36h1v1h-1z M17,36h1v1h-1z M19,36h1v1h-1z M20,36h1v1h-1z M24,36h1v1h-1z M26,36h1v1h-1z M29,36h1v1h-1z M32,36h1v1h-1z M33,36h1v1h-1z M35,36h1v1h-1z M40,36h1v1h-1z M43,36h1v1h-1z M44,36h1v1h-1z M49,36h1v1h-1z M5,37h1v1h-1z M7,37h1v1h-1z M8,37h1v1h-1z M9,37h1v1h-1z M12,37h1v1h-1z M14,37h1v1h-1z M15,37h1v1h-1z M16,37h1v1h-1z M18,37h1v1h-1z M21,37h1v1h-1z M23,37h1v1h-1z M27,37h1v1h-1z M28,37h1v1h-1z M29,37h1v1h-1z M30,37h1v1h-1z M31,37h1v1h-1z M32,37h1v1h-1z M34,37h1v1h-1z M35,37h1v1h-1z M39,37h1v1h-1z M40,37h1v1h-1z M41,37h1v1h-1z M43,37h1v1h-1z M44,37h1v1h-1z M46,37h1v1h-1z M47,37h1v1h-1z M49,37h1v1h-1z M7,38h1v1h-1z M8,38h1v1h-1z M10,38h1v1h-1z M11,38h1v1h-1z M14,38h1v1h-1z M16,38h1v1h-1z M17,38h1v1h-1z M19,38h1v1h-1z M20,38h1v1h-1z M21,38h1v1h-1z M22,38h1v1h-1z M24,38h1v1h-1z M25,38h1v1h-1z M26,38h1v1h-1z M29,38h1v1h-1z M32,38h1v1h-1z M33,38h1v1h-1z M36,38h1v1h-1z M38,38h1v1h-1z M39,38h1v1h-1z M41,38h1v1h-1z M42,38h1v1h-1z M43,38h1v1h-1z M46,38h1v1h-1z M47,38h1v1h-1z M48,38h1v1h-1z M49,38h1v1h-1z M51,38h1v1h-1z M52,38h1v1h-1z M5,39h1v1h-1z M6,39h1v1h-1z M7,39h1v1h-1z M8,39h1v1h-1z M11,39h1v1h-1z M12,39h1v1h-1z M15,39h1v1h-1z M18,39h1v1h-1z M22,39h1v1h-1z M25,39h1v1h-1z M27,39h1v1h-1z M28,39h1v1h-1z M32,39h1v1h-1z M33,39h1v1h-1z M36,39h1v1h-1z M41,39h1v1h-1z M42,39h1v1h-1z M44,39h1v1h-1z M45,39h1v1h-1z M52,39h1v1h-1z M6,40h1v1h-1z M9,40h1v1h-1z M10,40h1v1h-1z M11,40h1v1h-1z M14,40h1v1h-1z M15,40h1v1h-1z M16,40h1v1h-1z M17,40h1v1h-1z M18,40h1v1h-1z M20,40h1v1h-1z M21,40h1v1h-1z M23,40h1v1h-1z M24,40h1v1h-1z M27,40h1v1h-1z M28,40h1v1h-1z M29,40h1v1h-1z M30,40h1v1h-1z M33,40h1v1h-1z M35,40h1v1h-1z M37,40h1v1h-1z M39,40h1v1h-1z M40,40h1v1h-1z M41,40h1v1h-1z M42,40h1v1h-1z M43,40h1v1h-1z M44,40h1v1h-1z M45,40h1v1h-1z M48,40h1v1h-1z M49,40h1v1h-1z M50,40h1v1h-1z M5,41h1v1h-1z M6,41h1v1h-1z M9,41h1v1h-1z M11,41h1v1h-1z M12,41h1v1h-1z M20,41h1v1h-1z M23,41h1v1h-1z M25,41h1v1h-1z M27,41h1v1h-1z M28,41h1v1h-1z M29,41h1v1h-1z M33,41h1v1h-1z M34,41h1v1h-1z M36,41h1v1h-1z M41,41h1v1h-1z M43,41h1v1h-1z M45,41h1v1h-1z M46,41h1v1h-1z M47,41h1v1h-1z M50,41h1v1h-1z M51,41h1v1h-1z M5,42h1v1h-1z M9,42h1v1h-1z M10,42h1v1h-1z M17,42h1v1h-1z M19,42h1v1h-1z M22,42h1v1h-1z M23,42h1v1h-1z M27,42h1v1h-1z M28,42h1v1h-1z M31,42h1v1h-1z M32,42h1v1h-1z M35,42h1v1h-1z M36,42h1v1h-1z M39,42h1v1h-1z M41,42h1v1h-1z M42,42h1v1h-1z M45,42h1v1h-1z M46,42h1v1h-1z M48,42h1v1h-1z M51,42h1v1h-1z M52,42h1v1h-1z M5,43h1v1h-1z M6,43h1v1h-1z M7,43h1v1h-1z M11,43h1v1h-1z M13,43h1v1h-1z M14,43h1v1h-1z M22,43h1v1h-1z M24,43h1v1h-1z M25,43h1v1h-1z M26,43h1v1h-1z M28,43h1v1h-1z M30,43h1v1h-1z M31,43h1v1h-1z M33,43h1v1h-1z M34,43h1v1h-1z M39,43h1v1h-1z M41,43h1v1h-1z M48,43h1v1h-1z M49,43h1v1h-1z M51,43h1v1h-1z M4,44h1v1h-1z M5,44h1v1h-1z M6,44h1v1h-1z M10,44h1v1h-1z M11,44h1v1h-1z M15,44h1v1h-1z M16,44h1v1h-1z M18,44h1v1h-1z M22,44h1v1h-1z M23,44h1v1h-1z M26,44h1v1h-1z M27,44h1v1h-1z M28,44h1v1h-1z M29,44h1v1h-1z M30,44h1v1h-1z M34,44h1v1h-1z M35,44h1v1h-1z M37,44h1v1h-1z M39,44h1v1h-1z M40,44h1v1h-1z M41,44h1v1h-1z M43,44h1v1h-1z M44,44h1v1h-1z M45,44h1v1h-1z M46,44h1v1h-1z M47,44h1v1h-1z M48,44h1v1h-1z M50,44h1v1h-1z M52,44h1v1h-1z M12,45h1v1h-1z M13,45h1v1h-1z M15,45h1v1h-1z M17,45h1v1h-1z M18,45h1v1h-1z M19,45h1v1h-1z M21,45h1v1h-1z M23,45h1v1h-1z M26,45h1v1h-1z M30,45h1v1h-1z M33,45h1v1h-1z M34,45h1v1h-1z M39,45h1v1h-1z M41,45h1v1h-1z M43,45h1v1h-1z M44,45h1v1h-1z M48,45h1v1h-1z M50,45h1v1h-1z M51,45h1v1h-1z M4,46h1v1h-1z M5,46h1v1h-1z M6,46h1v1h-1z M7,46h1v1h-1z M8,46h1v1h-1z M9,46h1v1h-1z M10,46h1v1h-1z M16,46h1v1h-1z M18,46h1v1h-1z M19,46h1v1h-1z M21,46h1v1h-1z M22,46h1v1h-1z M25,46h1v1h-1z M26,46h1v1h-1z M28,46h1v1h-1z M30,46h1v1h-1z M31,46h1v1h-1z M34,46h1v1h-1z M35,46h1v1h-1z M40,46h1v1h-1z M42,46h1v1h-1z M44,46h1v1h-1z M46,46h1v1h-1z M48,46h1v1h-1z M51,46h1v1h-1z M52,46h1v1h-1z M4,47h1v1h-1z M10,47h1v1h-1z M12,47h1v1h-1z M13,47h1v1h-1z M14,47h1v1h-1z M15,47h1v1h-1z M16,47h1v1h-1z M17,47h1v1h-1z M21,47h1v1h-1z M22,47h1v1h-1z M25,47h1v1h-1z M26,47h1v1h-1z M30,47h1v1h-1z M31,47h1v1h-1z M32,47h1v1h-1z M34,47h1v1h-1z M36,47h1v1h-1z M37,47h1v1h-1z M38,47h1v1h-1z M39,47h1v1h-1z M41,47h1v1h-1z M42,47h1v1h-1z M43,47h1v1h-1z M44,47h1v1h-1z M48,47h1v1h-1z M51,47h1v1h-1z M52,47h1v1h-1z M4,48h1v1h-1z M6,48h1v1h-1z M7,48h1v1h-1z M8,48h1v1h-1z M10,48h1v1h-1z M12,48h1v1h-1z M15,48h1v1h-1z M16,48h1v1h-1z M17,48h1v1h-1z M19,48h1v1h-1z M21,48h1v1h-1z M22,48h1v1h-1z M23,48h1v1h-1z M26,48h1v1h-1z M27,48h1v1h-1z M28,48h1v1h-1z M29,48h1v1h-1z M30,48h1v1h-1z M33,48h1v1h-1z M39,48h1v1h-1z M40,48h1v1h-1z M41,48h1v1h-1z M42,48h1v1h-1z M44,48h1v1h-1z M45,48h1v1h-1z M46,48h1v1h-1z M47,48h1v1h-1z M48,48h1v1h-1z M49,48h1v1h-1z M4,49h1v1h-1z M6,49h1v1h-1z M7,49h1v1h-1z M8,49h1v1h-1z M10,49h1v1h-1z M12,49h1v1h-1z M13,49h1v1h-1z M15,49h1v1h-1z M17,49h1v1h-1z M19,49h1v1h-1z M22,49h1v1h-1z M25,49h1v1h-1z M26,49h1v1h-1z M29,49h1v1h-1z M30,49h1v1h-1z M31,49h1v1h-1z M32,49h1v1h-1z M33,49h1v1h-1z M34,49h1v1h-1z M36,49h1v1h-1z M40,49h1v1h-1z M41,49h1v1h-1z M43,49h1v1h-1z M44,49h1v1h-1z M45,49h1v1h-1z M46,49h1v1h-1z M47,49h1v1h-1z M48,49h1v1h-1z M49,49h1v1h-1z M50,49h1v1h-1z M52,49h1v1h-1z M4,50h1v1h-1z M6,50h1v1h-1z M7,50h1v1h-1z M8,50h1v1h-1z M10,50h1v1h-1z M12,50h1v1h-1z M15,50h1v1h-1z M17,50h1v1h-1z M18,50h1v1h-1z M21,50h1v1h-1z M23,50h1v1h-1z M26,50h1v1h-1z M27,50h1v1h-1z M28,50h1v1h-1z M30,50h1v1h-1z M32,50h1v1h-1z M33,50h1v1h-1z M35,50h1v1h-1z M37,50h1v1h-1z M38,50h1v1h-1z M39,50h1v1h-1z M40,50h1v1h-1z M42,50h1v1h-1z M47,50h1v1h-1z M49,50h1v1h-1z M4,51h1v1h-1z M10,51h1v1h-1z M14,51h1v1h-1z M15,51h1v1h-1z M17,51h1v1h-1z M18,51h1v1h-1z M19,51h1v1h-1z M20,51h1v1h-1z M26,51h1v1h-1z M27,51h1v1h-1z M28,51h1v1h-1z M29,51h1v1h-1z M30,51h1v1h-1z M31,51h1v1h-1z M34,51h1v1h-1z M36,51h1v1h-1z M37,51h1v1h-1z M39,51h1v1h-1z M43,51h1v1h-1z M46,51h1v1h-1z M47,51h1v1h-1z M52,51h1v1h-1z M4,52h1v1h-1z M5,52h1v1h-1z M6,52h1v1h-1z M7,52h1v1h-1z M8,52h1v1h-1z M9,52h1v1h-1z M10,52h1v1h-1z M12,52h1v1h-1z M15,52h1v1h-1z M16,52h1v1h-1z M18,52h1v1h-1z M21,52h1v1h-1z M22,52h1v1h-1z M23,52h1v1h-1z M24,52h1v1h-1z M31,52h1v1h-1z M34,52h1v1h-1z M35,52h1v1h-1z M37,52h1v1h-1z M38,52h1v1h-1z M40,52h1v1h-1z M41,52h1v1h-1z M42,52h1v1h-1z M49,52h1v1h-1z M51,52h1v1h-1z M52,52h1v1h-1z\" fill=\"#000000\"/>\n\t</g>\n</svg>)";
    QJsonArray codes;
    codes.append("testcode1");
    codes.append("testcode2");
    codes.append("testcode3");
    codes.append("testcode4");
    codes.append("testcode5");
    codes.append("testcode6");
    response["recovery_codes"] = codes;
    response["sign_up_success"] = 1;
    handleQrCodeResponse(response);
    // ================== [테스트용 QR코드/복구코드 표시 - 실제 배포 전 반드시 삭제] ==================
}

LoginWindow::~LoginWindow()
{
    qDebug() << "[LoginWindow] 소멸자 호출";

    if (m_tcpCommunicator) {
        m_tcpCommunicator->disconnectFromServer();
        m_tcpCommunicator->deleteLater();
        m_tcpCommunicator = nullptr;
    }

    if (m_connectionTimer) {
        m_connectionTimer->stop();
    }

    delete ui;

    qDebug() << "[LoginWindow] 소멸자 완료";
}

void LoginWindow::setupConnectionStatusLabel()
{
    // 연결 상태 표시 라벨 생성 (로그인 페이지 하단에)
    m_connectionStatusLabel = new QLabel(ui->LgoinPage);
    m_connectionStatusLabel->setText("서버 연결 중...");
    m_connectionStatusLabel->setStyleSheet(
        "QLabel {"
        "    color: #ff6b35;"
        "    font-size: 11px;"
        "    background-color: transparent;"
        "}"
        );
    m_connectionStatusLabel->setGeometry(40, 300, 200, 15); // 로그인 버튼 아래
    m_connectionStatusLabel->show();
}

void LoginWindow::setupConnectionTimer()
{
    // 연결 상태 확인 타이머 (5초마다 확인)
    m_connectionTimer = new QTimer(this);
    m_connectionTimer->setInterval(5000); // 5초
    connect(m_connectionTimer, &QTimer::timeout, this, &LoginWindow::checkConnectionStatus);
    m_connectionTimer->start();
}

void LoginWindow::checkConnectionStatus()
{
    qDebug() << "[LoginWindow] 연결 상태 확인";

    if (m_tcpCommunicator) {
        bool isConnected = m_tcpCommunicator->isConnectedToServer();
        qDebug() << "[LoginWindow] TCP 연결 상태:" << isConnected;

        if (isConnected) {
            m_connectionStatusLabel->setText("서버 연결됨");
            m_connectionStatusLabel->setStyleSheet(
                "QLabel {"
                "    color: #28a745;"
                "    font-size: 11px;"
                "    background-color: transparent;"
                "}"
                );
        } else {
            m_connectionStatusLabel->setText("서버 연결 끊어짐 - 재연결 시도 중...");
            m_connectionStatusLabel->setStyleSheet(
                "QLabel {"
                "    color: #dc3545;"
                "    font-size: 11px;"
                "    background-color: transparent;"
                "}"
                );

            // 재연결 시도
            m_tcpCommunicator->connectToServer(m_tcpHost, m_tcpPort);
        }
    } else {
        qDebug() << "[LoginWindow] TcpCommunicator가 초기화되지 않음";
    }
}

void LoginWindow::setupPasswordFields()
{
    qDebug() << "[LoginWindow] 패스워드 필드 설정";

    // 로그인 페이지 패스워드 필드
    ui->pwLineEdit->setEchoMode(QLineEdit::Password);

    // 회원가입 페이지 패스워드 필드들
    ui->pwLineEdit_3->setEchoMode(QLineEdit::Password);
    ui->pwLineEdit_4->setEchoMode(QLineEdit::Password);

    // 비밀번호 확인 필드에 실시간 검증 연결
    connect(ui->pwLineEdit_3, &QLineEdit::textChanged, this, &LoginWindow::onPasswordChanged);
    connect(ui->pwLineEdit_4, &QLineEdit::textChanged, this, &LoginWindow::onPasswordChanged);
}

void LoginWindow::setupPasswordErrorLabel()
{
    qDebug() << "[LoginWindow] 패스워드 오류 라벨 설정";

    // 비밀번호 오류 메시지 라벨 생성
    m_passwordErrorLabel = new QLabel(ui->SignUp);
    m_passwordErrorLabel->setText("* 비밀번호가 다릅니다");
    m_passwordErrorLabel->setStyleSheet("color: red; font-size: 12px;");
    m_passwordErrorLabel->setGeometry(40, 385, 200, 20); // SUBMIT 버튼 위에 위치
    m_passwordErrorLabel->hide(); // 초기에는 숨김
}

void LoginWindow::setupCloseButtons()
{
    qDebug() << "[LoginWindow] 닫기 버튼 설정";

    // Sign Up 페이지 닫기 버튼
    m_closeSignUpButton = new QPushButton("×", ui->SignUp);
    m_closeSignUpButton->setGeometry(295, 10, 25, 25); // 오른쪽 상단 모서리
    m_closeSignUpButton->setStyleSheet(
        "QPushButton {"
        "    background-color: transparent;"
        "    border: none;"
        "    color: #666;"
        "    font-size: 18px;"
        "    font-weight: bold;"
        "    border-radius: 12px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #e81123;"
        "    color: white;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #c50e1f;"
        "}"
        );

    // OTP Sign Up 페이지 닫기 버튼
    m_closeOtpSignUpButton = new QPushButton("×", ui->OTPSignUp);
    m_closeOtpSignUpButton->setGeometry(295, 10, 25, 25); // 오른쪽 상단 모서리
    m_closeOtpSignUpButton->setStyleSheet(
        "QPushButton {"
        "    background-color: transparent;"
        "    border: none;"
        "    color: #666;"
        "    font-size: 18px;"
        "    font-weight: bold;"
        "    border-radius: 12px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #e81123;"
        "    color: white;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #c50e1f;"
        "}"
        );
}

void LoginWindow::setupTcpCommunication()
{
    qDebug() << "[LoginWindow] TCP 통신 설정 시작";

    // TcpCommunicator 인스턴스 생성
    m_tcpCommunicator = new TcpCommunicator(this);

    // TCP 시그널 연결
    connect(m_tcpCommunicator, &TcpCommunicator::connected,
            this, &LoginWindow::onTcpConnected);
    connect(m_tcpCommunicator, &TcpCommunicator::disconnected,
            this, &LoginWindow::onTcpDisconnected);
    connect(m_tcpCommunicator, &TcpCommunicator::errorOccurred,
            this, &LoginWindow::onTcpError);
    connect(m_tcpCommunicator, &TcpCommunicator::messageReceived,
            this, &LoginWindow::onTcpMessageReceived);

    // 서버 연결 시도
    qDebug() << "[LoginWindow] 서버 연결 시도:" << m_tcpHost << ":" << m_tcpPort;
    m_tcpCommunicator->connectToServer(m_tcpHost, m_tcpPort);

    // 연결 상태 업데이트
    m_connectionStatusLabel->setText("서버 연결 시도 중...");
    m_connectionStatusLabel->setStyleSheet(
        "QLabel {"
        "    color: #ffc107;"
        "    font-size: 11px;"
        "    background-color: transparent;"
        "}"
        );

    qDebug() << "[LoginWindow] TCP 통신 설정 완료";
}

void LoginWindow::connectSignals()
{
    qDebug() << "[LoginWindow] 시그널 연결 시작";

    // 로그인 페이지 시그널 연결
    connect(ui->SubmitButton, &QPushButton::clicked,
            this, &LoginWindow::handleLogin);
    connect(ui->SignUpButton, &QPushButton::clicked,
            this, &LoginWindow::handleSignUpSwitch);

    // OTP 로그인 페이지 시그널 연결
    connect(ui->SubmitButton_2, &QPushButton::clicked,
            this, &LoginWindow::handleSubmitOtpLogin);
    connect(ui->SignUpButton_2, &QPushButton::clicked,
            this, &LoginWindow::handleSignUpSwitch);

    // 회원가입 페이지 시그널 연결
    connect(ui->SubmitButton_5, &QPushButton::clicked,
            this, &LoginWindow::handleSubmitSignUp);

    // OTP 회원가입 페이지 시그널 연결
    connect(ui->SubmitButton_6, &QPushButton::clicked,
            this, &LoginWindow::handleSubmitOtpSignUp);

    // 닫기 버튼 시그널 연결
    connect(m_closeSignUpButton, &QPushButton::clicked,
            this, &LoginWindow::handleCloseSignUp);
    connect(m_closeOtpSignUpButton, &QPushButton::clicked,
            this, &LoginWindow::handleCloseOtpSignUp);
}

void LoginWindow::handleCloseSignUp()
{
    qDebug() << "[LoginWindow] Sign Up 닫기 버튼 클릭";

    // 회원가입 창을 닫고 로그인 페이지로 돌아가기
    ui->stackedWidget->setCurrentIndex(0); // page_3 (로그인 페이지)
    ui->OTPLoginWidget->setCurrentIndex(0); // LgoinPage
    clearSignUpFields();
}

void LoginWindow::handleCloseOtpSignUp()
{
    qDebug() << "[LoginWindow] OTP Sign Up 닫기 버튼 클릭";

    // OTP 회원가입 창을 닫고 일반 회원가입 페이지로 돌아가기
    ui->OTPLoginWidget_3->setCurrentIndex(0); // SignUpPage
}

void LoginWindow::onPasswordChanged()
{
    qDebug() << "[LoginWindow] 패스워드 변경됨";

    QString password = ui->pwLineEdit_3->text();
    QString confirmPassword = ui->pwLineEdit_4->text();

    // 둘 다 비어있으면 오류 메시지 숨김
    if (password.isEmpty() && confirmPassword.isEmpty()) {
        m_passwordErrorLabel->hide();
        return;
    }

    // 확인 비밀번호가 비어있으면 오류 메시지 숨김
    if (confirmPassword.isEmpty()) {
        m_passwordErrorLabel->hide();
        return;
    }

    // 비밀번호가 다르면 오류 메시지 표시
    if (password != confirmPassword) {
        m_passwordErrorLabel->show();
    } else {
        m_passwordErrorLabel->hide();
    }
}

void LoginWindow::handleLogin()
{
    qDebug() << "[LoginWindow] 로그인 버튼 클릭";

    QString id = ui->idLineEdit->text().trimmed();
    QString password = ui->pwLineEdit->text().trimmed();

    if (id.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "입력 오류", "아이디와 패스워드를 모두 입력해주세요.");
        return;
    }

    // TCP 연결 확인
    if (!m_tcpCommunicator || !m_tcpCommunicator->isConnectedToServer()) {
        QMessageBox::warning(this, "연결 오류", "서버에 연결되지 않았습니다.\n잠시 후 다시 시도해주세요.");

        // 재연결 시도
        m_tcpCommunicator->connectToServer(m_tcpHost, m_tcpPort);
        return;
    }

    // 로그인 버튼 비활성화
    ui->SubmitButton->setEnabled(false);
    ui->SubmitButton->setText("로그인 중...");

    // 로그인 요청 전송
    sendLoginRequest(id, password);
}

void LoginWindow::handleSignUpSwitch()
{
    qDebug() << "[LoginWindow] Sign Up 전환";

    // 회원가입 페이지로 전환 (page_4)
    ui->stackedWidget->setCurrentIndex(1); // page_4 (회원가입 페이지)
    ui->OTPLoginWidget_3->setCurrentIndex(0); // SignUpPage

    // 입력 필드 초기화
    clearSignUpFields();
}

void LoginWindow::handleOtpSignupSwitch()
{
    qDebug() << "[LoginWindow] OTP Sign Up 전환";

    // OTP 회원가입 페이지로 전환
    ui->OTPLoginWidget_3->setCurrentIndex(1); // OTPSignUpPage
}

void LoginWindow::handleSubmitSignUp()
{
    qDebug() << "[LoginWindow] Sign Up 제출";

    QString id = ui->IDLabel->text().trimmed();
    QString password = ui->pwLineEdit_3->text().trimmed();
    QString confirmPassword = ui->pwLineEdit_4->text().trimmed();
    bool useOtp = ui->checkBox->isChecked();

    // 입력 유효성 검사
    if (id.isEmpty() || password.isEmpty() || confirmPassword.isEmpty()) {
        QMessageBox::warning(this, "입력 오류", "모든 필드를 입력해주세요.");
        return;
    }

    // 비밀번호 일치 검증
    if (password != confirmPassword) {
        m_passwordErrorLabel->show();
        QMessageBox::warning(this, "비밀번호 오류", "비밀번호가 일치하지 않습니다.");
        return;
    }

    // TCP 연결 확인
    if (!m_tcpCommunicator || !m_tcpCommunicator->isConnectedToServer()) {
        QMessageBox::warning(this, "연결 오류", "서버에 연결되지 않았습니다.\n잠시 후 다시 시도해주세요.");

        // 재연결 시도
        m_tcpCommunicator->connectToServer(m_tcpHost, m_tcpPort);
        return;
    }

    // 비밀번호가 일치하면 오류 메시지 숨김
    m_passwordErrorLabel->hide();

    sendSignUpRequest(id, password, useOtp);
}

void LoginWindow::handleSubmitOtpLogin()
{
    qDebug() << "[LoginWindow] OTP 로그인 제출";

    QString otpCode = ui->idLineEdit_2->text().trimmed();

    if (otpCode.isEmpty()) {
        QMessageBox::warning(this, "입력 오류", "OTP 코드를 입력해주세요.");
        return;
    }

    // TCP 연결 확인
    if (!m_tcpCommunicator || !m_tcpCommunicator->isConnectedToServer()) {
        QMessageBox::warning(this, "연결 오류", "서버에 연결되지 않았습니다.");
        resetOtpLoginButton();
        return;
    }

    // OTP 로그인 버튼 비활성화
    ui->SubmitButton_2->setEnabled(false);
    ui->SubmitButton_2->setText("인증 중...");

    // OTP 로그인 요청 전송
    sendOtpLoginRequest(otpCode);
}

void LoginWindow::handleSubmitOtpSignUp()
{
    qDebug() << "[LoginWindow] OTP 등록 완료 버튼 클릭. 로그인 페이지로 돌아감.";

    // 로그인 페이지로 돌아가기
    ui->stackedWidget->setCurrentIndex(0); // page_3 (로그인 페이지)
    ui->OTPLoginWidget->setCurrentIndex(0); // LgoinPage
    clearSignUpFields();
    clearLoginFields();

    // TCP 연결 확인
    if (!m_tcpCommunicator || !m_tcpCommunicator->isConnectedToServer()) {
        QMessageBox::warning(this, "연결 오류", "서버에 연결되지 않았습니다.");
        return;
    }

    // OTP 회원가입 요청 전송
    // sendOtpSignUpRequest(otpCode);
}

void LoginWindow::sendLoginRequest(const QString &id, const QString &password)
{
    qDebug() << "[LoginWindow] 로그인 요청 전송 - ID:" << id;

    // TCP 연결 확인
    if (!m_tcpCommunicator || !m_tcpCommunicator->isConnectedToServer()) {
        QMessageBox::warning(this, "연결 오류", "서버에 연결되지 않았습니다.");
        resetLoginButton();
        return;
    }

    // 로그인 요청 JSON 생성 (request_id: 8)
    QJsonObject loginMessage;
    loginMessage["request_id"] = 8;

    QJsonObject data;
    data["id"] = id;
    data["passwd"] = password;  // password -> passwd로 변경
    loginMessage["data"] = data;

    // 현재 로그인 정보 저장 (OTP 로그인용)
    m_currentUserId = id;
    m_currentPassword = password;

    qDebug() << "[LoginWindow] 전송할 JSON:" << QJsonDocument(loginMessage).toJson(QJsonDocument::Compact);

    // 서버로 전송
    bool success = m_tcpCommunicator->sendJsonMessage(loginMessage);
    if (!success) {
        QMessageBox::warning(this, "전송 오류", "로그인 정보 전송에 실패했습니다.");
        resetLoginButton();
    } else {
        qDebug() << "[LoginWindow] 로그인 요청 전송 성공";
    }
}

void LoginWindow::sendSignUpRequest(const QString &id, const QString &password, bool useOtp)
{
    qDebug() << "[LoginWindow] 회원가입 요청 전송 - ID:" << id << "OTP:" << useOtp;

    // TCP 연결 확인
    if (!m_tcpCommunicator || !m_tcpCommunicator->isConnectedToServer()) {
        QMessageBox::warning(this, "연결 오류", "서버에 연결되지 않았습니다.");
        return;
    }

    // 회원가입 요청 JSON 생성 (request_id: 9)
    QJsonObject signUpMessage;
    signUpMessage["request_id"] = 9;

    QJsonObject data;
    data["id"] = id;
    data["passwd"] = password;  // password -> passwd로 변경
    data["use_otp"] = useOtp;
    signUpMessage["data"] = data;

    qDebug() << "[LoginWindow] 전송할 JSON:" << QJsonDocument(signUpMessage).toJson(QJsonDocument::Compact);

    // 서버로 전송
    bool success = m_tcpCommunicator->sendJsonMessage(signUpMessage);
    if (!success) {
        QMessageBox::warning(this, "전송 오류", "회원가입 정보 전송에 실패했습니다.");
    } else {
        qDebug() << "[LoginWindow] 회원가입 요청 전송 성공";
    }
}

void LoginWindow::sendOtpLoginRequest(const QString &otpCode)
{
    qDebug() << "[LoginWindow] OTP 2차 로그인 요청 전송";

    // TCP 연결 확인
    if (!m_tcpCommunicator || !m_tcpCommunicator->isConnectedToServer()) {
        QMessageBox::warning(this, "연결 오류", "서버에 연결되지 않았습니다.");
        resetOtpLoginButton();
        return;
    }

    // OTP 로그인 요청 JSON 생성 (request_id: 22)
    QJsonObject otpLoginMessage;
    otpLoginMessage["request_id"] = 22;
    QJsonObject dataMessage;
    dataMessage["id"] = m_currentUserId;
    dataMessage["input"] = otpCode;
    otpLoginMessage.insert("data",dataMessage);

    // 서버로 전송
    bool success = m_tcpCommunicator->sendJsonMessage(otpLoginMessage);
    if (!success) {
        QMessageBox::warning(this, "전송 오류", "OTP 인증 정보 전송에 실패했습니다.");
        resetOtpLoginButton();
    }

    qDebug() << "[LoginWindow] OTP 2차 로그인 요청 전송 성공 - ID:" << m_currentUserId;
}

// void LoginWindow::sendOtpSignUpRequest(const QString &otpCode)
// {
//     qDebug() << "[LoginWindow] OTP 회원가입 요청 전송";

//     // TCP 연결 확인
//     if (!m_tcpCommunicator || !m_tcpCommunicator->isConnectedToServer()) {
//         QMessageBox::warning(this, "연결 오류", "서버에 연결되지 않았습니다.");
//         return;
//     }

//     // OTP 회원가입 완료 요청 JSON 생성 (request_id: 1004)
//     QJsonObject otpSignUpMessage;
//     otpSignUpMessage["request_id"] = 1004;
//     otpSignUpMessage["otp_code"] = otpCode;

//     // 서버로 전송
//     bool success = m_tcpCommunicator->sendJsonMessage(otpSignUpMessage);
//     if (!success) {
//         QMessageBox::warning(this, "전송 오류", "OTP 회원가입 정보 전송에 실패했습니다.");
//     }

//     qDebug() << "[LoginWindow] OTP 회원가입 요청 전송";
// }

// void LoginWindow::generateOtpQrCode(const QString &id, const QString &password)
// {
//     qDebug() << "[LoginWindow] OTP QR 코드 생성 요청";

//     // TCP 연결 확인
//     if (!m_tcpCommunicator || !m_tcpCommunicator->isConnectedToServer()) {
//         QMessageBox::warning(this, "연결 오류", "서버에 연결되지 않았습니다.");
//         return;
//     }

//     // OTP QR 코드 생성 요청 JSON (request_id: 1005)
//     QJsonObject qrCodeMessage;
//     qrCodeMessage["request_id"] = 1005;
//     qrCodeMessage["id"] = id;
//     qrCodeMessage["passwd"] = password;  // password -> passwd로 변경

//     // 현재 회원가입 정보 저장
//     m_currentUserId = id;
//     m_currentPassword = password;

//     // 서버로 전송
//     bool success = m_tcpCommunicator->sendJsonMessage(qrCodeMessage);
//     if (!success) {
//         QMessageBox::warning(this, "전송 오류", "OTP QR 코드 요청에 실패했습니다.");
//     }

//     qDebug() << "[LoginWindow] OTP QR 코드 생성 요청 전송 - ID:" << id;
// }

void LoginWindow::resetLoginButton()
{
    ui->SubmitButton->setEnabled(true);
    ui->SubmitButton->setText("SUBMIT");
}

void LoginWindow::resetOtpLoginButton()
{
    ui->SubmitButton_2->setEnabled(true);
    ui->SubmitButton_2->setText("SUBMIT");
}

void LoginWindow::clearSignUpFields()
{
    ui->IDLabel->clear();
    ui->pwLineEdit_3->clear();
    ui->pwLineEdit_4->clear();
    ui->checkBox->setChecked(false);
    m_passwordErrorLabel->hide(); // 오류 메시지도 숨김
}

void LoginWindow::clearLoginFields()
{
    ui->idLineEdit->clear();
    ui->pwLineEdit->clear();
    ui->idLineEdit_2->clear();
    ui->lineEdit->clear();
}

void LoginWindow::onTcpConnected()
{
    qDebug() << "[LoginWindow] TCP 연결 성공";

    // 연결 상태 업데이트
    m_connectionStatusLabel->setText("서버 연결됨");
    m_connectionStatusLabel->setStyleSheet(
        "QLabel {"
        "    color: #28a745;"
        "    font-size: 11px;"
        "    background-color: transparent;"
        "}"
        );
}

void LoginWindow::onTcpDisconnected()
{
    qDebug() << "[LoginWindow] TCP 연결 해제";

    // 연결 상태 업데이트
    m_connectionStatusLabel->setText("서버 연결 끊어짐");
    m_connectionStatusLabel->setStyleSheet(
        "QLabel {"
        "    color: #dc3545;"
        "    font-size: 11px;"
        "    background-color: transparent;"
        "}"
        );

    // 사용자에게 알림 (한 번만)
    static bool disconnectNotified = false;
    if (!disconnectNotified) {
        QMessageBox::warning(this, "연결 해제", "서버와의 연결이 끊어졌습니다.\n자동으로 재연결을 시도합니다.");
        disconnectNotified = true;

        // 5초 후 알림 플래그 리셋
        QTimer::singleShot(5000, [&]() {
            disconnectNotified = false;
        });
    }
}

void LoginWindow::onTcpError(const QString &error)
{
    qDebug() << "[LoginWindow] TCP 오류:" << error;

    // 연결 상태 업데이트
    m_connectionStatusLabel->setText("연결 오류 - 재시도 중...");
    m_connectionStatusLabel->setStyleSheet(
        "QLabel {"
        "    color: #dc3545;"
        "    font-size: 11px;"
        "    background-color: transparent;"
        "}"
        );

    // 버튼 상태 복원
    resetLoginButton();
    resetOtpLoginButton();

    // 오류 로그 출력
    qDebug() << "[LoginWindow] 상세 오류 정보:" << error;
}

void LoginWindow::onTcpMessageReceived(const QString &message)
{
    qDebug() << "[LoginWindow] TCP 메시지 수신:" << message;

    // JSON 파싱
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8(), &error);

    if (error.error != QJsonParseError::NoError) {
        qDebug() << "[LoginWindow] JSON 파싱 오류:" << error.errorString();
        qDebug() << "[LoginWindow] 원본 메시지:" << message;
        return;
    }

    QJsonObject jsonObj = doc.object();
    int requestId = jsonObj["request_id"].toInt();

    qDebug() << "[LoginWindow] 파싱된 request_id:" << requestId;

    // 응답 처리
    switch (requestId) {
    case 19: // 로그인 응답
        qDebug() << "[LoginWindow] 로그인 응답 처리";
        handleLoginResponse(jsonObj);
        break;
    case 20: // 회원가입 응답
        qDebug() << "[LoginWindow] 회원가입 응답 처리";
        handleSignUpResponse(jsonObj);
        break;
    case 23:
        qDebug() << "[LoginWindow] 2차 로그인 응답 처리";
        break;
    // case 10: // OTP 로그인 응답 (필요시)
    //     qDebug() << "[LoginWindow] OTP 로그인 응답 처리";
    //     handleOtpLoginResponse(jsonObj);
    //     break;
    // case 11: // OTP 회원가입 응답 (필요시)
    //     qDebug() << "[LoginWindow] OTP 회원가입 응답 처리";
    //     handleOtpSignUpResponse(jsonObj);
    //     break;
    // case 12: // OTP QR 코드 응답 (필요시)
    //     qDebug() << "[LoginWindow] QR 코드 응답 처리";
    //     handleQrCodeResponse(jsonObj);
    //     break;
    default:
        qDebug() << "[LoginWindow] 알 수 없는 request_id:" << requestId;
        qDebug() << "[LoginWindow] 전체 JSON:" << QJsonDocument(jsonObj).toJson(QJsonDocument::Compact);
        break;
    }
}

void LoginWindow::handleLoginResponse(const QJsonObject &response)
{
    int success = response["step1_success"].toInt();
    QString message = response["message"].toString();
    int isOtpUser = response["requires_otp"].toInt(0);

    qDebug() << "[LoginWindow] 로그인 응답 - 성공:" << success << "메시지:" << message << "OTP 사용자:" << isOtpUser;

    resetLoginButton();

    if (success != 0) {
        if (isOtpUser == 1) {
            // OTP 사용자인 경우 OTP 로그인 페이지로 전환
            ui->OTPLoginWidget->setCurrentIndex(1); // OTPLoginPage
            ui->idLineEdit_2->setFocus();
            QMessageBox::information(this, "OTP 인증", "OTP 인증번호를 입력해주세요.");
        } else {
            // 일반 사용자인 경우 로그인 성공
            QMessageBox::information(this, "로그인 성공", "로그인에 성공했습니다.");
            emit loginSuccessful();
            accept();
        }
    } else {
        QMessageBox::warning(this, "로그인 실패", message.isEmpty() ? "로그인에 실패했습니다." : message);
        clearLoginFields();
    }
}

void LoginWindow::handleSignUpResponse(const QJsonObject &response)
{
    int success = response["sign_up_success"].toInt();
    QString message = response["message"].toString();
    QString qrCodeSvg = response["qr_code_svg"].toString();
    QString otpURI = response["otp_uri"].toString();
    QString recoveryCodes = response["recovery_codes"].toString();

    if (success != 0) {
        QMessageBox::information(this, "회원가입 성공", "회원가입이 완료되었습니다. QR 코드를 통해 OTP를 등록해주십시오.");
        handleOtpSignupSwitch();
    } else {
        QMessageBox::warning(this, "회원가입 실패", message.isEmpty() ? "회원가입에 실패했습니다." : message);
    }

    qDebug() << "[LoginWindow] 회원가입 응답 - 성공:" << success << "메시지:" << message;

}

void LoginWindow::handleOtpLoginResponse(const QJsonObject &response)
{
    bool success = response["success"].toBool();
    QString message = response["message"].toString();

    resetOtpLoginButton();

    if (success) {
        QMessageBox::information(this, "로그인 성공", "OTP 인증이 완료되었습니다.");
        emit loginSuccessful();
        accept();
    } else {
        QMessageBox::warning(this, "OTP 인증 실패", message.isEmpty() ? "OTP 인증에 실패했습니다." : message);
        ui->idLineEdit_2->clear();
        ui->idLineEdit_2->setFocus();
    }
}

void LoginWindow::handleOtpSignUpResponse(const QJsonObject &response)
{
    bool success = response["success"].toBool();
    QString message = response["message"].toString();
    

    if (success) {
        QMessageBox::information(this, "회원가입 성공", "OTP 회원가입이 완료되었습니다.");
        // 로그인 페이지로 돌아가기
        ui->stackedWidget->setCurrentIndex(0); // page_3 (로그인 페이지)
        ui->OTPLoginWidget->setCurrentIndex(0); // LgoinPage
        clearSignUpFields();
        clearLoginFields();
    } else {
        QMessageBox::warning(this, "OTP 회원가입 실패", message.isEmpty() ? "OTP 회원가입에 실패했습니다." : message);
    }
}

void LoginWindow::handleQrCodeResponse(const QJsonObject &response)
{
    QString svgData = response["qr_code_svg"].toString();
    QJsonArray recoveryCodesArray = response["recovery_codes"].toArray();
    int signUpSuccess = response["sign_up_success"].toInt();

    svgData.replace("\\n", "\n");
    svgData.replace("\\\"", "\""); // svg의 이스케이프 등에 대한 파싱

    if (signUpSuccess == 1 && !svgData.isEmpty()) {
        QSvgRenderer svgRenderer(svgData.toUtf8());
        QPixmap pixmap(211, 201); // frame 크기와 동일
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        svgRenderer.render(&painter);

        // QR코드 라벨 생성 및 표시
        QFrame *frame = ui->OTPSignUp->findChild<QFrame*>("frame");
        if (frame) {
            // 기존 라벨 제거
            QLabel *oldLabel = frame->findChild<QLabel*>("qrCodeLabel");
            if (oldLabel) delete oldLabel;

            QLabel *qrLabel = new QLabel(frame);
            qrLabel->setObjectName("qrCodeLabel");
            qrLabel->setPixmap(pixmap);
            qrLabel->setGeometry(0, 0, 211, 201); // frame 내부 좌표
            qrLabel->setAlignment(Qt::AlignCenter);
            qrLabel->show();
        }

        // 복구코드 라벨 생성 및 표시
        QStringList codeList;
        for (const QJsonValue &v : recoveryCodesArray)
            codeList << v.toString();

        // 기존 QScrollArea 및 QLabel 제거 (중복 방지)
        QScrollArea *oldScroll = ui->OTPSignUp->findChild<QScrollArea*>("recoveryScrollArea");
        if (oldScroll) delete oldScroll;

        QScrollArea *scrollArea = new QScrollArea(ui->OTPSignUp);
        scrollArea->setObjectName("recoveryScrollArea");
        scrollArea->setGeometry(60, 320, 211, 65); // 버튼과 겹치지 않게 높이 조정
        scrollArea->setWidgetResizable(true);
        scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);


        QTextEdit *recoveryEdit = new QTextEdit;
        recoveryEdit->setObjectName("recoveryEdit");
        recoveryEdit->setText(codeList.join("\n"));
        recoveryEdit->setStyleSheet("color: #333; font-size: 13px; background: #f5f5f5; border: 1px solid #ccc; border-radius: 6px; padding: 6px;");
        recoveryEdit->setAlignment(Qt::AlignLeft);
        recoveryEdit->setReadOnly(true);
        recoveryEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        recoveryEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        recoveryEdit->setMinimumHeight(65);
        recoveryEdit->setMaximumHeight(65); // 스크롤바 완전 차단
        scrollArea->setWidget(recoveryEdit);
        scrollArea->show();

        QMessageBox::information(this, "OTP 설정", "QR코드를 OTP 앱으로 스캔하고, 복구 코드를 안전하게 보관하세요.");
    } else {
        QMessageBox::warning(this, "QR 코드 생성 실패", "QR 코드 생성에 실패했습니다.");
        ui->OTPLoginWidget_3->setCurrentIndex(0); // SignUpPage로 복귀
    }
}
