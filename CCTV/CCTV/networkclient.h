#ifndef NETWORKCLIENT_H
#define NETWORKCLIENT_H

#include <QObject>              // Qt 객체 기본 클래스
#include <QTcpSocket>           // TCP 소켓 클래스
#include <QUdpSocket>           // UDP 소켓 클래스
#include <QTimer>               // 타이머 클래스
#include <QHostAddress>         // 호스트 주소 클래스
#include <QByteArray>           // 바이트 배열 클래스
#include <QDataStream>          // 데이터 스트림 클래스

// 메시지 타입 정의
enum MessageType {
    MSG_VIDEO_FRAME = 0x01,     // 비디오 프레임 데이터
    MSG_CAPTURED_IMAGE = 0x02,  // 캡처된 이미지 데이터
    MSG_COMMAND = 0x03,         // 명령어 메시지
    MSG_STATUS = 0x04,          // 상태 메시지
    MSG_HEARTBEAT = 0x05        // 하트비트 메시지
};

// 명령어 타입 정의
enum CommandType {
    CMD_START_STREAM = 0x10,    // 스트리밍 시작
    CMD_STOP_STREAM = 0x11,     // 스트리밍 정지
    CMD_CAPTURE_IMAGE = 0x12,   // 이미지 캡처 요청
    CMD_LCD_CONTROL = 0x13,     // LCD 제어
    CMD_GET_STATUS = 0x14       // 상태 요청
};

// 네트워크 클라이언트 클래스
class NetworkClient : public QObject
{
    Q_OBJECT

public:
    explicit NetworkClient(QObject *parent = nullptr);   // 생성자
    ~NetworkClient();                                    // 소멸자

    // 연결 관리
    void connectToServer(const QString &host, quint16 port);    // 서버 연결
    void disconnectFromServer();                                // 서버 연결 해제
    bool isConnected() const;                                   // 연결 상태 확인

    // 명령어 전송
    void sendCommand(CommandType command, const QByteArray &data = QByteArray());  // 명령어 전송
    void requestImageCapture();                                 // 이미지 캡처 요청
    void controlLCD(int lcdId, bool state);                     // LCD 제어 명령
    void startVideoStream();                                    // 비디오 스트리밍 시작 요청
    void stopVideoStream();                                     // 비디오 스트리밍 정지 요청

    // 설정
    void setHeartbeatInterval(int interval);                    // 하트비트 간격 설정

signals:
    // 연결 관련 시그널
    void connected();                                           // 연결 완료 시그널
    void disconnected();                                        // 연결 해제 시그널
    void connectionError(const QString &error);                 // 연결 에러 시그널

    // 데이터 수신 시그널
    void videoFrameReceived(const QByteArray &frameData);       // 비디오 프레임 수신
    void capturedImageReceived(const QByteArray &imageData);    // 캡처 이미지 수신
    void statusReceived(const QString &status);                 // 상태 메시지 수신

private slots:
    void onConnected();                                         // 연결 완료 처리
    void onDisconnected();                                      // 연결 해제 처리
    void onError(QAbstractSocket::SocketError error);           // 소켓 에러 처리
    void onReadyRead();                                         // 데이터 수신 처리
    void onHeartbeatTimer();                                    // 하트비트 타이머

private:
    // 내부 함수들
    void processMessage(const QByteArray &message);             // 메시지 처리
    void sendMessage(MessageType type, const QByteArray &data); // 메시지 전송
    QByteArray createMessage(MessageType type, const QByteArray &data); // 메시지 생성

    // 멤버 변수들
    QTcpSocket *m_tcpSocket;                                    // TCP 소켓
    QTimer *m_heartbeatTimer;                                   // 하트비트 타이머
    QByteArray m_receiveBuffer;                                 // 수신 버퍼
    quint32 m_expectedMessageSize;                              // 예상 메시지 크기
    bool m_messageHeaderReceived;                               // 메시지 헤더 수신 여부
};

#endif // NETWORKCLIENT_H
