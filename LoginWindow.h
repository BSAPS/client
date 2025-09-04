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

class LoginWindow : public QDialog
{
    Q_OBJECT

public:
    explicit LoginWindow(QWidget *parent = nullptr);
    ~LoginWindow();

    // 화면 상태 확인 메서드
    bool isLoginWindowVisible() const;
    bool isLoginWindowHidden() const;
    QString getLoginWindowStatus() const;

    // TCP 통신기 공유 메서드
    TcpCommunicator* getTcpCommunicator() const;
    void setTcpCommunicator(TcpCommunicator* communicator);

private slots:
    // UI 이벤트 핸들러
    void handleLogin();
    void handleSignUpSwitch();
    void handleOtpSignupSwitch();
    void handleSubmitSignUp();
    void handleSubmitOtpLogin();
    void handleSubmitOtpSignUp();
    void handleCloseSignUp();
    void handleCloseOtpSignUp();
    void onPasswordChanged();

    // 연결 상태 확인
    void checkConnectionStatus();

    // TCP 통신 관련 슬롯
    void onTcpConnected();
    void onTcpDisconnected();
    void onTcpError(const QString &error);
    void onTcpMessageReceived(const QString &message);

private:
    Ui_LoginWindow *ui;

    // TCP 통신
    TcpCommunicator *m_tcpCommunicator;

    // UI 컴포넌트
    QLabel *m_passwordErrorLabel;
    QPushButton *m_closeSignUpButton;
    QPushButton *m_closeOtpSignUpButton;
    QLabel *m_connectionStatusLabel;
    QTimer *m_connectionTimer;

    // 사용자 데이터
    QString m_currentUserId;
    QString m_currentPassword;

    // 네트워크 설정 (.env에서 로드)
    QString m_tcpHost;
    quint16 m_tcpPort;

    // 초기화 메서드
    void setupPasswordFields();
    void setupPasswordErrorLabel();
    void setupCloseButtons();
    void setupConnectionStatusLabel();
    void setupConnectionTimer();
    void setupTcpCommunication();
    void connectSignals();

    // TCP 통신 메서드
    void sendLoginRequest(const QString &id, const QString &password);
    void sendSignUpRequest(const QString &id, const QString &password, bool useOtp = false);
    void sendOtpLoginRequest(const QString &otpCode);
    void sendOtpSignUpRequest(const QString &otpCode);
    void generateOtpQrCode(const QString &id, const QString &password);

    // 응답 처리 메서드
    void handleLoginResponse(const QJsonObject &response);
    void handleSignUpResponse(const QJsonObject &response);
    void handleOtpLoginResponse(const QJsonObject &response);
    void handleOtpSignUpResponse(const QJsonObject &response);
    void handleQrCodeResponse(const QJsonObject &response);

    // 유틸리티 메서드
    void resetLoginButton();
    void resetOtpLoginButton();
    void clearSignUpFields();
    void clearLoginFields();

    CustomTitleBar *titleBar;
};

#endif // LOGINWINDOW_H
