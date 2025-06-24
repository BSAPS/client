#include "networkclient.h"
#include <QDebug>               // 디버그 출력
#include <QDataStream>          // 데이터 스트림

NetworkClient::NetworkClient(QObject *parent)
    : QObject(parent)
    , m_tcpSocket(nullptr)
    , m_heartbeatTimer(nullptr)
    , m_expectedMessageSize(0)
    , m_messageHeaderReceived(false)
{
    // TCP 소켓 초기화
    m_tcpSocket = new QTcpSocket(this);
    
    // 소켓 시그널 연결
    connect(m_tcpSocket, &QTcpSocket::connected, this, &NetworkClient::onConnected);
    connect(m_tcpSocket, &QTcpSocket::disconnected, this, &NetworkClient::onDisconnected);
    connect(m_tcpSocket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
            this, &NetworkClient::onError);
    connect(m_tcpSocket, &QTcpSocket::readyRead, this, &NetworkClient::onReadyRead);
    
    // 하트비트 타이머 초기화
    m_heartbeatTimer = new QTimer(this);
    connect(m_heartbeatTimer, &QTimer::timeout, this, &NetworkClient::onHeartbeatTimer);
    setHeartbeatInterval(5000);         // 기본 5초 간격
}

NetworkClient::~NetworkClient()
{
    disconnectFromServer();             // 연결 해제
}

void NetworkClient::connectToServer(const QString &host, quint16 port)
{
    if (m_tcpSocket->state() == QAbstractSocket::ConnectedState) {
        qDebug() << "Already connected to server";
        return;
    }
    
    qDebug() << "Connecting to server:" << host << ":" << port;
    m_tcpSocket->connectToHost(host, port);     // 서버 연결 시도
}

void NetworkClient::disconnectFromServer()
{
    if (m_heartbeatTimer->isActive()) {
        m_heartbeatTimer->stop();               // 하트비트 타이머 정지
    }
    
    if (m_tcpSocket->state() == QAbstractSocket::ConnectedState) {
        m_tcpSocket->disconnectFromHost();      // 서버 연결 해제
    }
}

bool NetworkClient::isConnected() const
{
    return m_tcpSocket->state() == QAbstractSocket::ConnectedState;
}

void NetworkClient::sendCommand(CommandType command, const QByteArray &data)
{
    if (!isConnected()) {
        qWarning() << "Not connected to server";
        return;
    }
    
    // 명령어 데이터 구성
    QByteArray commandData;
    QDataStream stream(&commandData, QIODevice::WriteOnly);
    stream << static_cast<quint8>(command);     // 명령어 타입
    stream.writeRawData(data.constData(), data.size());  // 추가 데이터
    
    sendMessage(MSG_COMMAND, commandData);      // 명령어 메시지 전송
    qDebug() << "Command sent:" << command;
}

void NetworkClient::requestImageCapture()
{
    sendCommand(CMD_CAPTURE_IMAGE);             // 이미지 캡처 명령 전송
    qDebug() << "Image capture requested";
}

void NetworkClient::controlLCD(int lcdId, bool state)
{
    QByteArray lcdData;
    QDataStream stream(&lcdData, QIODevice::WriteOnly);
    stream << static_cast<quint8>(lcdId);       // LCD ID
    stream << static_cast<quint8>(state ? 1 : 0);  // 상태 (ON/OFF)
    
    sendCommand(CMD_LCD_CONTROL, lcdData);      // LCD 제어 명령 전송
    qDebug() << "LCD control sent - ID:" << lcdId << "State:" << (state ? "ON" : "OFF");
}

void NetworkClient::startVideoStream()
{
    sendCommand(CMD_START_STREAM);              // 스트리밍 시작 명령
    qDebug() << "Video stream start requested";
}

void NetworkClient::stopVideoStream()
{
    sendCommand(CMD_STOP_STREAM);               // 스트리밍 정지 명령
    qDebug() << "Video stream stop requested";
}

void NetworkClient::setHeartbeatInterval(int interval)
{
    m_heartbeatTimer->setInterval(interval);    // 하트비트 간격 설정
}

void NetworkClient::onConnected()
{
    qDebug() << "Connected to server successfully";
    m_heartbeatTimer->start();                  // 하트비트 시작
    emit connected();                           // 연결 완료 시그널
}

void NetworkClient::onDisconnected()
{
    qDebug() << "Disconnected from server";
    m_heartbeatTimer->stop();                   // 하트비트 정지
    m_receiveBuffer.clear();                    // 수신 버퍼 클리어
    m_messageHeaderReceived = false;            // 헤더 수신 상태 초기화
    m_expectedMessageSize = 0;
    emit disconnected();                        // 연결 해제 시그널
}

void NetworkClient::onError(QAbstractSocket::SocketError error)
{
    QString errorString;
    switch (error) {
    case QAbstractSocket::ConnectionRefusedError:
        errorString = "Connection refused by server";
        break;
    case QAbstractSocket::HostNotFoundError:
        errorString = "Server host not found";
        break;
    case QAbstractSocket::NetworkError:
        errorString = "Network error occurred";
        break;
    default:
        errorString = "Unknown network error";
        break;
    }
    
    qWarning() << "Network error:" << errorString;
    emit connectionError(errorString);          // 에러 시그널 발생
}

void NetworkClient::onReadyRead()
{
    // 사용 가능한 모든 데이터 읽기
    QByteArray newData = m_tcpSocket->readAll();
    m_receiveBuffer.append(newData);
    
    // 메시지 처리 루프
    while (true) {
        if (!m_messageHeaderReceived) {
            // 헤더 크기 확인 (메시지 타입 1바이트 + 크기 4바이트 = 5바이트)
            if (m_receiveBuffer.size() < 5) {
                break;  // 헤더가 완전하지 않음
            }
            
            // 헤더에서 메시지 크기 추출
            QDataStream headerStream(m_receiveBuffer);
            quint8 messageType;
            headerStream >> messageType >> m_expectedMessageSize;
            
            m_messageHeaderReceived = true;
        }
        
        // 전체 메시지 크기 확인 (헤더 5바이트 + 데이터)
        quint32 totalMessageSize = 5 + m_expectedMessageSize;
        if (m_receiveBuffer.size() < totalMessageSize) {
            break;  // 메시지가 완전하지 않음
        }
        
        // 완전한 메시지 추출
        QByteArray completeMessage = m_receiveBuffer.left(totalMessageSize);
        m_receiveBuffer.remove(0, totalMessageSize);
        
        // 메시지 처리
        processMessage(completeMessage);
        
        // 다음 메시지를 위한 상태 초기화
        m_messageHeaderReceived = false;
        m_expectedMessageSize = 0;
    }
}

void NetworkClient::onHeartbeatTimer()
{
    if (isConnected()) {
        sendMessage(MSG_HEARTBEAT, QByteArray());   // 하트비트 메시지 전송
    }
}

void NetworkClient::processMessage(const QByteArray &message)
{
    if (message.size() < 5) {
        qWarning() << "Invalid message size:" << message.size();
        return;
    }
    
    // 메시지 헤더 파싱
    QDataStream stream(message);
    quint8 messageType;
    quint32 dataSize;
    stream >> messageType >> dataSize;
    
    // 데이터 부분 추출
    QByteArray data = message.mid(5);
    
    // 메시지 타입에 따른 처리
    switch (static_cast<MessageType>(messageType)) {
    case MSG_VIDEO_FRAME:
        emit videoFrameReceived(data);          // 비디오 프레임 시그널
        break;
        
    case MSG_CAPTURED_IMAGE:
        emit capturedImageReceived(data);       // 캡처 이미지 시그널
        qDebug() << "Captured image received, size:" << data.size();
        break;
        
    case MSG_STATUS:
        emit statusReceived(QString::fromUtf8(data));   // 상태 메시지 시그널
        break;
        
    case MSG_HEARTBEAT:
        // 하트비트 응답 (특별한 처리 불필요)
        break;
        
    default:
        qWarning() << "Unknown message type:" << messageType;
        break;
    }
}

void NetworkClient::sendMessage(MessageType type, const QByteArray &data)
{
    if (!isConnected()) {
        return;
    }
    
    QByteArray message = createMessage(type, data);     // 메시지 생성
    qint64 bytesWritten = m_tcpSocket->write(message);  // 메시지 전송
    
    if (bytesWritten == -1) {
        qWarning() << "Failed to send message";
    }
}

QByteArray NetworkClient::createMessage(MessageType type, const QByteArray &data)
{
    QByteArray message;
    QDataStream stream(&message, QIODevice::WriteOnly);
    
    // 메시지 헤더 작성
    stream << static_cast<quint8>(type);                // 메시지 타입
    stream << static_cast<quint32>(data.size());        // 데이터 크기
    
    // 데이터 추가
    message.append(data);
    
    return message;
}
