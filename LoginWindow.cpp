#include "LoginWindow.h"
#include "ui_LoginWindow.h"
#include "MainWindow.h"
#include "TcpCommunicator.h"
#include "EnvConfig.h"
#include "CustomTitleBar.h"
#include "CustomMessageBox.h"

#include <QApplication>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonArray>
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
{
    ui->setupUi(this);
    qDebug() << "[LoginWindow] 생성자 시작";

    // 기본 타이틀바 숨기기
    setWindowFlags(Qt::FramelessWindowHint);

    titleBar = new CustomTitleBar(this);
    titleBar->setTitle("Login & SignUp Window");

    // 시그널과 슬롯 연결
    connect(titleBar, &CustomTitleBar::minimizeClicked, this, &MainWindow::showMinimized);
    connect(titleBar, &CustomTitleBar::closeClicked, this, &QMainWindow::close);

    // 전체 레이아웃 설정
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(titleBar);
    mainLayout->addWidget(ui->stackedWidget); // 기존 ui 파일의 최상위 위젯을 레이아웃에 추가

    setLayout(mainLayout); // 다이얼로그에 레이아웃 적용

    // .env 파일 로드
    EnvConfig::loadFromFile();
    
    // 네트워크 설정 초기화
    m_tcpHost = EnvConfig::getValue("TCP_HOST", "192.168.0.81");
    m_tcpPort = EnvConfig::getValue("TCP_PORT", "8080").toUInt();
    
    qDebug() << "[LoginWindow] .env 설정 로드 - TCP_HOST:" << m_tcpHost << "TCP_PORT:" << m_tcpPort;

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
    ui->stackedWidget->setCurrentIndex(0); // page_3 (로그인 페이지)
    ui->OTPLoginWidget->setCurrentIndex(0); // LoginPage

    // 다이얼로그 설정
    setModal(true);
    setWindowTitle("CCTV 모니터링 시스템 - 로그인");

    // 연결 상태 확인 타이머 설정
    setupConnectionTimer(); // 서버 구동시 주석해제

    qDebug() << "[LoginWindow] 생성자 완료";
}

LoginWindow::~LoginWindow()
{
    qDebug() << "[LoginWindow] 소멸자 호출";


    if (m_connectionTimer) {
        m_connectionTimer->stop();
    }

    delete ui;

    qDebug() << "[LoginWindow] 소멸자 완료";
}

// 화면 상태 확인 메서드들
bool LoginWindow::isLoginWindowVisible() const
{
    return isVisible();
}

bool LoginWindow::isLoginWindowHidden() const
{
    return isHidden();
}

QString LoginWindow::getLoginWindowStatus() const
{
    if (isVisible()) {
        return "LoginWindow가 현재 표시되고 있습니다.";
    } else {
        return "LoginWindow가 숨겨져 있습니다.";
    }
}

// TCP 통신기 공유 메서드들
TcpCommunicator* LoginWindow::getTcpCommunicator() const
{
    return m_tcpCommunicator;
}

void LoginWindow::setTcpCommunicator(TcpCommunicator* communicator)
{
    // 기존 연결 해제
    if (m_tcpCommunicator && m_tcpCommunicator != communicator) {
        disconnect(m_tcpCommunicator, &TcpCommunicator::connected,
                  this, &LoginWindow::onTcpConnected);
        disconnect(m_tcpCommunicator, &TcpCommunicator::disconnected,
                  this, &LoginWindow::onTcpDisconnected);
        disconnect(m_tcpCommunicator, &TcpCommunicator::errorOccurred,
                  this, &LoginWindow::onTcpError);
        disconnect(m_tcpCommunicator, &TcpCommunicator::messageReceived,
                  this, &LoginWindow::onTcpMessageReceived);
    }

    m_tcpCommunicator = communicator;

    // 새로운 통신기에 시그널 연결
    if (m_tcpCommunicator) {
        connect(m_tcpCommunicator, &TcpCommunicator::connected,
                this, &LoginWindow::onTcpConnected);
        connect(m_tcpCommunicator, &TcpCommunicator::disconnected,
                this, &LoginWindow::onTcpDisconnected);
        connect(m_tcpCommunicator, &TcpCommunicator::errorOccurred,
                this, &LoginWindow::onTcpError);
        connect(m_tcpCommunicator, &TcpCommunicator::messageReceived,
                this, &LoginWindow::onTcpMessageReceived);
    }
}

void LoginWindow::setupConnectionStatusLabel()
{
    // 연결 상태 표시 라벨 생성 (로그인 페이지 하단에)
    m_connectionStatusLabel = new QLabel(ui->LoginPage);
    m_connectionStatusLabel->setGeometry(40, 285, 200, 15); // 로그인 버튼 아래
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
    ui->OTPLoginWidget->setCurrentIndex(0); // LoginPage
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
        CustomMessageBox msgBox(nullptr, "입력 오류", "아이디와 패스워드를 모두 입력해주세요.");
        msgBox.exec(); // 모달(modal)로 띄워서 사용자의 응답을 기다림
        return;
    }

    // TCP 연결 확인
    if (!m_tcpCommunicator || !m_tcpCommunicator->isConnectedToServer()) {
        CustomMessageBox msgBox(nullptr, "연결 오류", "서버에 연결되지 않았습니다.\n잠시 후 다시 시도해주세요.");
        msgBox.exec();

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
        CustomMessageBox msgBox(nullptr, "입력 오류", "모든 필드를 입력해주세요.");
        msgBox.exec();
        return;
    }

    // 비밀번호 일치 검증
    if (password != confirmPassword) {
        m_passwordErrorLabel->show();
        CustomMessageBox msgBox(nullptr, "비밀번호 오류", "비밀번호가 일치하지 않습니다.");
        msgBox.exec();
        return;
    }

    // TCP 연결 확인
    if (!m_tcpCommunicator || !m_tcpCommunicator->isConnectedToServer()) {
        CustomMessageBox msgBox(nullptr, "연결 오류", "서버에 연결되지 않았습니다.\n잠시 후 다시 시도해주세요.");
        msgBox.exec();

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
        CustomMessageBox msgBox(nullptr, "입력 오류", "OTP 코드를 입력해주세요.");
        msgBox.exec();
        return;
    }

    // TCP 연결 확인
    if (!m_tcpCommunicator || !m_tcpCommunicator->isConnectedToServer()) {
        CustomMessageBox msgBox(nullptr, "연결 오류", "서버에 연결되지 않았습니다.");
        msgBox.exec();
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
    ui->OTPLoginWidget->setCurrentIndex(0); // LoginPage
    clearSignUpFields();
    clearLoginFields();

    // TCP 연결 확인
    if (!m_tcpCommunicator || !m_tcpCommunicator->isConnectedToServer()) {
        CustomMessageBox msgBox(nullptr, "연결 오류", "서버에 연결되지 않았습니다.");
        msgBox.exec();
        return;
    }

}

void LoginWindow::sendLoginRequest(const QString &id, const QString &password)
{
    qDebug() << "[LoginWindow] 로그인 요청 전송 - ID:" << id;

    // TCP 연결 확인
    if (!m_tcpCommunicator || !m_tcpCommunicator->isConnectedToServer()) {
        CustomMessageBox msgBox(nullptr, "연결 오류", "서버에 연결되지 않았습니다.");
        msgBox.exec();
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
        CustomMessageBox msgBox(nullptr, "전송 오류", "로그인 정보 전송에 실패했습니다.");
        msgBox.exec();
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
        CustomMessageBox msgBox(nullptr, "연결 오류", "서버에 연결되지 않았습니다.");
        msgBox.exec();
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
        CustomMessageBox msgBox(nullptr, "전송 오류", "회원가입 정보 전송에 실패했습니다.");
        msgBox.exec();
    } else {
        qDebug() << "[LoginWindow] 회원가입 요청 전송 성공";
    }
}

void LoginWindow::sendOtpLoginRequest(const QString &otpCode)
{
    qDebug() << "[LoginWindow] OTP 2차 로그인 요청 전송";

    // TCP 연결 확인
    if (!m_tcpCommunicator || !m_tcpCommunicator->isConnectedToServer()) {
        CustomMessageBox msgBox(nullptr, "연결 오류", "서버에 연결되지 않았습니다.");
        msgBox.exec();
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
        CustomMessageBox msgBox(nullptr, "전송 오류", "OTP 인증 정보 전송에 실패했습니다.");
        msgBox.exec();
        resetOtpLoginButton();
    }

    qDebug() << "[LoginWindow] OTP 2차 로그인 요청 전송 성공 - ID:" << m_currentUserId;
}


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
    if (!disconnectNotified && this->isVisible()) {
        CustomMessageBox msgBox(nullptr, "전송 오류", "OTP 인증 정보 전송에 실패했습니다.");
        msgBox.exec();
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
        handleOtpLoginResponse(jsonObj);
        break;

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
            CustomMessageBox msgBox(nullptr, "OTP 인증", "OTP 인증번호를 입력해주세요.");
            msgBox.exec();
        } else {
            // 일반 사용자인 경우 로그인 성공
            CustomMessageBox msgBox(nullptr, "로그인 성공", "로그인에 성공했습니다.");
            msgBox.exec();
            accept();
        }
    } else {
        CustomMessageBox msgBox(nullptr, "로그인 실패", message.isEmpty() ? "로그인에 실패했습니다." : message);
        msgBox.exec();
        clearLoginFields();
    }
}

void LoginWindow::handleSignUpResponse(const QJsonObject &response)
{
    int success = response["sign_up_success"].toInt();
    QString message = response["message"].toString();
    QString qrCodeSvg = response["qr_code_svg"].toString();
    QString otpURI = response["otp_uri"].toString();
    QJsonArray recoveryCodes = response["recovery_codes"].toArray();

    if (success != 0) {
        CustomMessageBox msgBox(nullptr, "회원가입 성공", "회원가입이 완료되었습니다.");
        msgBox.exec();
        if(otpURI != ""){
            handleOtpSignupSwitch();
        } else {
            ui->stackedWidget->setCurrentIndex(0);
        }
    } else {
        CustomMessageBox msgBox(nullptr, "회원가입 실패", message.isEmpty() ? "회원가입에 실패했습니다." : message);
        msgBox.exec();
    }

    QJsonObject res;
    res["qr_code_svg"] = qrCodeSvg;
    res["recovery_codes"] = recoveryCodes;
    res["sign_up_success"] = 1;
    if(otpURI != ""){
        handleQrCodeResponse(res);
    }

    qDebug() << "[LoginWindow] 회원가입 응답 - 성공:" << success << "메시지:" << message;

}

void LoginWindow::handleOtpLoginResponse(const QJsonObject &response)
{
    int success = response["final_login_success"].toInt();
    QString message = response["message"].toString();

    resetOtpLoginButton();

    if (success == 1) {
        CustomMessageBox msgBox(nullptr, "로그인 성공", message);
        msgBox.exec();
        accept();
    } else {
        CustomMessageBox msgBox(nullptr, "OTP 인증 실패", message.isEmpty() ? "OTP 인증에 실패했습니다." : message);
        msgBox.exec();
        ui->idLineEdit_2->clear();
        ui->idLineEdit_2->setFocus();
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

        CustomMessageBox msgBox(nullptr, "OTP 설정", "QR코드를 OTP 앱으로 스캔하고, \n복구 코드를 안전하게 보관하세요. \n복구 코드는 계정당 5개 주어지며, \n한 번 사용한 코드는 다시 사용할 수 없습니다.");
        msgBox.exec();
    } else {
        CustomMessageBox msgBox(nullptr, "QR 코드 생성 실패", "QR 코드 생성에 실패했습니다.");
        msgBox.exec();
        ui->OTPLoginWidget_3->setCurrentIndex(0); // SignUpPage로 복귀
    }
}
