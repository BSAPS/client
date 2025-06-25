#ifndef LCDCONTROLLER_H
#define LCDCONTROLLER_H

// Qt 네트워크 및 기본 헤더 파일들
#include <QObject>              // Qt 객체 기본 클래스
#include <QTcpSocket>           // TCP 소켓 클래스
#include <QTimer>               // 타이머 클래스
#include <QMap>                 // 맵 컨테이너 클래스

class LCDController : public QObject
{
    Q_OBJECT

public:
    explicit LCDController(QObject *parent = nullptr);  // 생성자
    ~LCDController();                                   // 소멸자

    // LCD 제어 관련 공개 함수들
    void controlLCD(int lcdId, bool turnOn);            // LCD 제어 (켜기/끄기)
    bool getLCDState(int lcdId) const;                  // LCD 상태 조회
    void setLCDServerAddress(const QString &address, quint16 port); // 서버 주소 설정
    
    // 시뮬레이션 모드 설정 (데모용)
    void simulateControl(bool enabled) { m_simulateMode = enabled; }

signals:
    // 시그널 함수들 (이벤트 알림용)
    void lcdStateChanged(int lcdId, bool state);        // LCD 상태 변경 시그널
    void connectionError(const QString &error);         // 연결 에러 시그널

private slots:
    // 네트워크 관련 슬롯 함수들
    void onConnected();                                 // 서버 연결 완료 처리
    void onDisconnected();                              // 서버 연결 해제 처리
    void onError(QAbstractSocket::SocketError error);   // 소켓 에러 처리
    void onReadyRead();                                 // 데이터 수신 처리

private:
    // 내부 함수들
    void sendCommand(const QString &command);           // 명령어 전송
    void connectToServer();                             // 서버 연결 시도

    // 내부 멤버 변수들
    QTcpSocket *m_socket;                               // TCP 소켓 객체
    QString m_serverAddress;                            // 서버 주소
    quint16 m_serverPort;                               // 서버 포트
    QMap<int, bool> m_lcdStates;                        // LCD 상태 저장 맵
    bool m_simulateMode;                                // 시뮬레이션 모드 플래그
    QTimer *m_reconnectTimer;                           // 재연결 타이머
};

#endif // LCDCONTROLLER_H
