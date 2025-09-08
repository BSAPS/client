#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QDialog>
#include <QObject>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QStackedWidget>
#include <QTimer>
#include <QJsonObject>

class QVBoxLayout;
class CustomTitleBar;
class TcpCommunicator;

QT_BEGIN_NAMESPACE
class Ui_LoginWindow;
QT_END_NAMESPACE

/**
 * @brief 로그인/회원가입 윈도우 클래스
 * @details 로그인, 회원가입, OTP 인증 등 인증 관련 UI와 로직을 담당합니다.
 */
class LoginWindow : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief LoginWindow 생성자
     * @param parent 부모 위젯
     */
    explicit LoginWindow(QWidget *parent = nullptr);

    /**
     * @brief LoginWindow 소멸자
     */
    ~LoginWindow();

    // 화면 상태 확인 메서드
    /**
     * @brief 로그인 창이 보이는지 확인
     * @return 보이면 true
     */
    bool isLoginWindowVisible() const;
    /**
     * @brief 로그인 창이 숨겨졌는지 확인
     * @return 숨겨졌으면 true
     */
    bool isLoginWindowHidden() const;
    /**
     * @brief 로그인 창 상태 문자열 반환
     * @return 상태 문자열
     */
    QString getLoginWindowStatus() const;

    // TCP 통신기 공유 메서드
    /**
     * @brief TCP 통신기 반환
     * @return TcpCommunicator 포인터
     */
    TcpCommunicator* getTcpCommunicator() const;
    /**
     * @brief TCP 통신기 설정
     * @param communicator TCP 통신 객체
     */
    void setTcpCommunicator(TcpCommunicator* communicator);

private slots:
    // UI 이벤트 핸들러
    /** @brief 로그인 버튼 클릭 슬롯 */
    void handleLogin();
    /** @brief 회원가입 전환 슬롯 */
    void handleSignUpSwitch();
    /** @brief OTP 회원가입 전환 슬롯 */
    void handleOtpSignupSwitch();
    /** @brief 회원가입 제출 슬롯 */
    void handleSubmitSignUp();
    /** @brief OTP 로그인 제출 슬롯 */
    void handleSubmitOtpLogin();
    /** @brief OTP 회원가입 제출 슬롯 */
    void handleSubmitOtpSignUp();
    /** @brief 회원가입 닫기 슬롯 */
    void handleCloseSignUp();
    /** @brief OTP 회원가입 닫기 슬롯 */
    void handleCloseOtpSignUp();
    /** @brief 비밀번호 변경 슬롯 */
    void onPasswordChanged();

    // 연결 상태 확인
    /** @brief 연결 상태 확인 슬롯 */
    void checkConnectionStatus();

    // TCP 통신 관련 슬롯
    /** @brief TCP 연결 성공 슬롯 */
    void onTcpConnected();
    /** @brief TCP 연결 해제 슬롯 */
    void onTcpDisconnected();
    /**
     * @brief TCP 에러 슬롯
     * @param error 에러 메시지
     */
    void onTcpError(const QString &error);
    /**
     * @brief TCP 메시지 수신 슬롯
     * @param message 수신 메시지
     */
    void onTcpMessageReceived(const QString &message);

private:
    /** @brief UI 객체 */
    Ui_LoginWindow *ui;

    // TCP 통신
    /** @brief TCP 통신 객체 */
    TcpCommunicator *m_tcpCommunicator;

    // UI 컴포넌트
    /** @brief 비밀번호 오류 라벨 */
    QLabel *m_passwordErrorLabel;
    /** @brief 회원가입 닫기 버튼 */
    QPushButton *m_closeSignUpButton;
    /** @brief OTP 회원가입 닫기 버튼 */
    QPushButton *m_closeOtpSignUpButton;
    /** @brief 연결 상태 라벨 */
    QLabel *m_connectionStatusLabel;
    /** @brief 연결 상태 확인 타이머 */
    QTimer *m_connectionTimer;

    // 사용자 데이터
    /** @brief 현재 사용자 ID */
    QString m_currentUserId;
    /** @brief 현재 비밀번호 */
    QString m_currentPassword;

    // 네트워크 설정 (.env에서 로드)
    /** @brief TCP 호스트 */
    QString m_tcpHost;
    /** @brief TCP 포트 */
    quint16 m_tcpPort;

    // 초기화 메서드
    /** @brief 패스워드 필드 설정 */
    void setupPasswordFields();
    /** @brief 패스워드 오류 라벨 설정 */
    void setupPasswordErrorLabel();
    /** @brief 닫기 버튼 설정 */
    void setupCloseButtons();
    /** @brief 연결 상태 라벨 설정 */
    void setupConnectionStatusLabel();
    /** @brief 연결 상태 타이머 설정 */
    void setupConnectionTimer();
    /** @brief TCP 통신 설정 */
    void setupTcpCommunication();
    /** @brief 시그널 연결 */
    void connectSignals();

    // TCP 통신 메서드
    /**
     * @brief 로그인 요청 전송
     * @param id 사용자 ID
     * @param password 비밀번호
     */
    void sendLoginRequest(const QString &id, const QString &password);
    /**
     * @brief 회원가입 요청 전송
     * @param id 사용자 ID
     * @param password 비밀번호
     * @param useOtp OTP 사용 여부
     */
    void sendSignUpRequest(const QString &id, const QString &password, bool useOtp = false);
    /**
     * @brief OTP 로그인 요청 전송
     * @param otpCode OTP 코드
     */
    void sendOtpLoginRequest(const QString &otpCode);
    /**
     * @brief OTP 회원가입 요청 전송
     * @param otpCode OTP 코드
     */
    void sendOtpSignUpRequest(const QString &otpCode);
    /**
     * @brief OTP QR코드 생성
     * @param id 사용자 ID
     * @param password 비밀번호
     */
    void generateOtpQrCode(const QString &id, const QString &password);

    // 응답 처리 메서드
    /**
     * @brief 로그인 응답 처리
     * @param response 응답 JSON
     */
    void handleLoginResponse(const QJsonObject &response);
    /**
     * @brief 회원가입 응답 처리
     * @param response 응답 JSON
     */
    void handleSignUpResponse(const QJsonObject &response);
    /**
     * @brief OTP 로그인 응답 처리
     * @param response 응답 JSON
     */
    void handleOtpLoginResponse(const QJsonObject &response);
    /**
     * @brief QR코드 응답 처리
     * @param response 응답 JSON
     */
    void handleQrCodeResponse(const QJsonObject &response);

    // 유틸리티 메서드
    /** @brief 로그인 버튼 초기화 */
    void resetLoginButton();
    /** @brief OTP 로그인 버튼 초기화 */
    void resetOtpLoginButton();
    /** @brief 회원가입 입력 필드 초기화 */
    void clearSignUpFields();
    /** @brief 로그인 입력 필드 초기화 */
    void clearLoginFields();

    /** @brief 커스텀 타이틀바 */
    CustomTitleBar *titleBar;
};

#endif // LOGINWINDOW_H
