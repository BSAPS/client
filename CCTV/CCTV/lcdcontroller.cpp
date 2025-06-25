#include "lcdcontroller.h"
#include <QDebug>               // 디버그 출력용

LCDController::LCDController(QObject *parent)
    : QObject(parent)
    , m_socket(nullptr)
    , m_serverAddress("127.0.0.1")     // 기본 서버 주소 (로컬호스트)
    , m_serverPort(8080)               // 기본 서버 포트
    , m_simulateMode(true)             // 기본적으로 시뮬레이션 모드 활성화
    , m_reconnectTimer(nullptr)
{
    // LCD 상태 초기화 (모든 LCD를 OFF 상태로 설정)
    for (int i = 1; i <= 4; ++i) {
        m_lcdStates[i] = false;         // LCD ID 1~4를 false(OFF)로 초기화
    }
    
    // TCP 소켓 생성
    m_socket = new QTcpSocket(this);
    // 소켓 시그널들과 슬롯 함수들 연결
    connect(m_socket, &QTcpSocket::connected, this, &LCDController::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &LCDController::onDisconnected);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
            this, &LCDController::onError);
    connect(m_socket, &QTcpSocket::readyRead, this, &LCDController::onReadyRead);
    
    // 재연결 타이머 생성
    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setSingleShot(true);              // 한 번만 실행되는 타이머로 설정
    connect(m_reconnectTimer, &QTimer::timeout, this, &LCDController::connectToServer);
    
    // 서버 연결 시도 (실패 시 시뮬레이션 모드로 전환)
    connectToServer();
}

LCDController::~LCDController()
{
    // 소켓이 연결된 상태면 연결 해제
    if (m_socket && m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->disconnectFromHost();
    }
}

void LCDController::controlLCD(int lcdId, bool turnOn)
{
    // LCD ID 유효성 검사 (1~4 범위)
    if (lcdId < 1 || lcdId > 4) {
        qWarning() << "Invalid LCD ID:" << lcdId;       // 경고 메시지 출력
        return;
    }
    
    m_lcdStates[lcdId] = turnOn;                        // LCD 상태 업데이트
    
    if (m_simulateMode) {
        // 시뮬레이션 모드: 실제 하드웨어 없이 동작 시뮬레이션
        qDebug() << QString("LCD %1 %2 (Simulated)").arg(lcdId).arg(turnOn ? "ON" : "OFF");
        emit lcdStateChanged(lcdId, turnOn);            // 상태 변경 시그널 발생
    } else {
        // 실제 모드: 서버에 명령어 전송
        QString command = QString("LCD%1:%2").arg(lcdId).arg(turnOn ? "ON" : "OFF");
        sendCommand(command);                           // 명령어 전송
    }
}

bool LCDController::getLCDState(int lcdId) const
{
    // 해당 LCD ID의 상태 반환 (없으면 기본값 false)
    return m_lcdStates.value(lcdId, false);
}

void LCDController::setLCDServerAddress(const QString &address, quint16 port)
{
    m_serverAddress = address;                          // 서버 주소 설정
    m_serverPort = port;                                // 서버 포트 설정
    
    // 현재 연결된 상태면 연결 해제
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->disconnectFromHost();
    }
    
    connectToServer();                                  // 새 주소로 연결 시도
}

void LCDController::sendCommand(const QString &command)
{
    // 소켓이 연결된 상태인지 확인
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        QByteArray data = command.toUtf8() + "\n";      // 명령어를 UTF-8로 변환하고 개행 추가
        m_socket->write(data);                          // 데이터 전송
        qDebug() << "Sent command:" << command;         // 전송 로그 출력
    } else {
        qWarning() << "Not connected to LCD server, command ignored:" << command;
    }
}

void LCDController::connectToServer()
{
    // 소켓이 연결되지 않은 상태인 경우에만 연결 시도
    if (m_socket->state() == QAbstractSocket::UnconnectedState) {
        qDebug() << "Attempting to connect to LCD server at" << m_serverAddress << ":" << m_serverPort;
        m_socket->connectToHost(m_serverAddress, m_serverPort); // 서버 연결 시도
    }
}

void LCDController::onConnected()
{
    qDebug() << "Connected to LCD server";              // 연결 성공 로그
    m_simulateMode = false;                             // 시뮬레이션 모드 해제
    emit connectionError("");                           // 이전 에러 메시지 클리어
}

void LCDController::onDisconnected()
{
    qDebug() << "Disconnected from LCD server, switching to simulation mode";
    m_simulateMode = true;                              // 시뮬레이션 모드로 전환
    
    // 5초 후 재연결 시도
    m_reconnectTimer->start(5000);
}

void LCDController::onError(QAbstractSocket::SocketError error)
{
    QString errorString;        // 에러 메시지 문자열
    switch (error) {            // 에러 타입에 따른 메시지 설정
    case QAbstractSocket::ConnectionRefusedError:
        errorString = "LCD server connection refused";  // 연결 거부 에러
        break;
    case QAbstractSocket::HostNotFoundError:
        errorString = "LCD server host not found";      // 호스트를 찾을 수 없음
        break;
    case QAbstractSocket::NetworkError:
        errorString = "Network error occurred";         // 네트워크 에러
        break;
    default:
        errorString = "Unknown network error";          // 알 수 없는 네트워크 에러
        break;
    }
    
    qWarning() << "LCD Controller error:" << errorString;   // 에러 로그 출력
    m_simulateMode = true;                              // 시뮬레이션 모드로 전환
    emit connectionError(errorString);                  // 에러 시그널 발생
    
    // 10초 후 재연결 시도
    m_reconnectTimer->start(10000);
}

void LCDController::onReadyRead()
{
    QByteArray data = m_socket->readAll();              // 수신된 모든 데이터 읽기
    QString response = QString::fromUtf8(data).trimmed(); // UTF-8로 변환하고 공백 제거
    qDebug() << "Received from LCD server:" << response; // 수신 로그 출력
    
    // 응답 메시지 파싱
    // 예상 형식: "LCD1:OK" 또는 "LCD1:ERROR"
    if (response.contains(":")) {                       // 콜론이 포함된 경우
        QStringList parts = response.split(":");        // 콜론으로 분리
        if (parts.size() == 2) {                        // 정확히 2개 부분으로 나뉜 경우
            QString lcdPart = parts[0];                 // LCD 부분 (예: "LCD1")
            QString statusPart = parts[1];              // 상태 부분 (예: "OK")
            
            if (lcdPart.startsWith("LCD")) {            // "LCD"로 시작하는 경우
                int lcdId = lcdPart.mid(3).toInt();     // "LCD" 이후 숫자 추출
                if (lcdId >= 1 && lcdId <= 4) {         // 유효한 LCD ID 범위인지 확인
                    if (statusPart == "OK") {           // 성공 응답인 경우
                        emit lcdStateChanged(lcdId, m_lcdStates[lcdId]); // 상태 변경 시그널 발생
                    } else {                            // 실패 응답인 경우
                        qWarning() << "LCD command failed for LCD" << lcdId; // 에러 로그 출력
                    }
                }
            }
        }
    }
}
