#include "TcpCommunicator.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>

#include "LineDrawingDialog.h"

TcpCommunicator::TcpCommunicator(QObject *parent)
    : QObject(parent)
    , m_socket(new QSslSocket(this))
    , m_connectionTimer(nullptr)
    , m_reconnectTimer(new QTimer(this))
    , m_host("")
    , m_port(0)
    , m_isConnected(false)
    , m_receivedData("")
    , m_videoView(nullptr)

    , m_connectionTimeoutMs(10000)
    , m_reconnectEnabled(true)
    , m_reconnectAttempts(0)
    , m_maxReconnectAttempts(5)
    , m_reconnectDelayMs(3000)
    , m_autoReconnect(true)

    , m_roadLinesReceived(false)
    , m_detectionLinesReceived(false)
{
    qDebug() << "[TCP] TcpCommunicator 생성자 호출";
    m_socket = new QSslSocket(this);
    setupSslConfiguration();

    // Keep-Alive settings
    m_socket->setSocketOption(QAbstractSocket::KeepAliveOption, 1);
    m_socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);

    // TCP 소켓 시그널 연결
    connect(m_socket, &QSslSocket::connected, this, &TcpCommunicator::onConnected);
    connect(m_socket, &QSslSocket::disconnected, this, &TcpCommunicator::onDisconnected);
    connect(m_socket, &QSslSocket::readyRead, this, &TcpCommunicator::onReadyRead);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QSslSocket::errorOccurred),
            this, &TcpCommunicator::onSocketError);

    // Connect SSL signals
    connect(m_socket, &QSslSocket::encrypted, this, &TcpCommunicator::onSslEncrypted);
    connect(m_socket, QOverload<const QList<QSslError>&>::of(&QSslSocket::sslErrors),
            this, &TcpCommunicator::onSslErrors);

    // Connection timeout timer
    m_connectionTimer = new QTimer(this);
    m_connectionTimer->setSingleShot(true);
    m_connectionTimer->setInterval(m_connectionTimeoutMs);
    connect(m_connectionTimer, &QTimer::timeout, this, &TcpCommunicator::onConnectionTimeout);

    // 재연결 타이머 설정
    m_reconnectTimer->setSingleShot(true);
    m_reconnectTimer->setInterval(m_reconnectDelayMs);
    connect(m_reconnectTimer, &QTimer::timeout, this, &TcpCommunicator::onReconnectTimer);

    qDebug() << "[TCP] TcpCommunicator 초기화 완료";
}

TcpCommunicator::~TcpCommunicator()
{
    disconnectFromServer();
}

void TcpCommunicator::disconnectFromServer()
{
    if (m_connectionTimer->isActive()) {
        m_connectionTimer->stop();
    }
    if (m_reconnectTimer->isActive()) {
        m_reconnectTimer->stop();
    }

    if (m_socket && m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->disconnectFromHost();
        if (m_socket->state() != QAbstractSocket::UnconnectedState) {
            m_socket->waitForDisconnected(3000);
        }
    }
}

void TcpCommunicator::setupSslConfiguration() {
    // Register server's CA certificate (e.g., ca-cert.pem file)
    QSslConfiguration sslConfiguration = QSslConfiguration::defaultConfiguration();

    // 2. Load server's CA certificate
    QList<QSslCertificate> caCerts = QSslCertificate::fromPath(":/ca-cert.crt");
    if (!caCerts.isEmpty()) {
        qDebug() << "[TCP] CA certificates loaded:" << caCerts.size() << "items";
        // 3. Add loaded CA certificates to SSL configuration
        sslConfiguration.setCaCertificates(caCerts);
    } else {
        qDebug() << "[TCP] Warning: ca-cert.crt file not found or could not be read.";
        qDebug() << "[TCP] SSL 인증서 검증을 완화하여 연결을 시도합니다.";
    }

    // 4. Apply SSL configuration to the socket
    m_socket->setSslConfiguration(sslConfiguration);

    // 5. Set server certificate verification mode
    // 개발 환경에서는 VerifyNone으로 설정하여 연결 문제 해결
    m_socket->setPeerVerifyMode(QSslSocket::VerifyNone);
    qDebug() << "[TCP] SSL Peer verification mode set to VerifyNone for development";
}

void TcpCommunicator::connectToServer(const QString &host, quint16 port)
{
    qDebug() << "[TCP] connectToServer 호출 - 호스트:" << host << "포트:" << port;

    m_host = host;
    m_port = port;

    // 이미 연결되어 있으면 연결 해제 후 재연결
    if (m_socket->state() != QSslSocket::UnconnectedState) {
        qDebug() << "[TCP] 기존 연결 해제 중... 현재 상태:" << m_socket->state();
        m_socket->disconnectFromHost();
        if (m_socket->state() != QSslSocket::UnconnectedState) {
            m_socket->waitForDisconnected(1000);
        }
    }

    qDebug() << "[TCP] 서버 연결 시도:" << host << ":" << port;
    qDebug() << "[TCP] SSL 설정 확인 - Peer Verify Mode:" << m_socket->peerVerifyMode();
    
    m_socket->connectToHostEncrypted(m_host, static_cast<qint16>(m_port));

    // 연결 타임아웃 설정 (10초)
    if (!m_socket->waitForConnected(10000)) {
        qDebug() << "[TCP] 연결 타임아웃 또는 실패:" << m_socket->errorString();
        emit errorOccurred("연결 타임아웃: " + m_socket->errorString());
    } else {
        qDebug() << "[TCP] TCP 연결 성공, SSL 핸드셰이크 대기 중...";
    }
}

bool TcpCommunicator::isConnectedToServer() const
{
    return m_isConnected && m_socket && m_socket->state() == QAbstractSocket::ConnectedState;
}

bool TcpCommunicator::sendJsonMessage(const QJsonObject &message)
{
    if (!isConnectedToServer()) {
        qDebug() << "[TCP] 메시지 전송 실패 - 서버에 연결되지 않음";
        return false;
    }

    QJsonDocument doc(message);
    QByteArray data = doc.toJson(QJsonDocument::Compact);

    // qDebug() << "[TCP] JSON 메시지 전송 시도:" << data;

    // 1. 데이터 길이(4바이트, 빅엔디안) 전송
    quint32 dataLength = static_cast<quint32>(data.size());
    QByteArray lengthBytes;
    QDataStream lengthStream(&lengthBytes, QIODevice::WriteOnly);
    lengthStream.setByteOrder(QDataStream::BigEndian);
    lengthStream << dataLength;
    
    // 2. 길이 + 데이터 순서로 전송
    qint64 bytesWritten = m_socket->write(lengthBytes);
    if (bytesWritten != 4) {
        qDebug() << "[TCP] 길이 정보 전송 실패:" << m_socket->errorString();
        return false;
    }
    bytesWritten = m_socket->write(data);
    bool flushed = m_socket->flush();

    if (bytesWritten == -1) {
        qDebug() << "[TCP] 메시지 전송 실패:" << m_socket->errorString();
        return false;
    }

    qDebug() << "[TCP] 메시지 전송 성공 - 바이트:" << bytesWritten << "플러시:" << flushed;
    return true;
}

bool TcpCommunicator::sendMessage(const QString &message)
{
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8(), &error);

    if (error.error != QJsonParseError::NoError) {
        qDebug() << "[TCP] JSON 파싱 오류:" << error.errorString();
        return false;
    }

    return sendJsonMessage(doc.object());
}

bool TcpCommunicator::sendLineCoordinates(int x1, int y1, int x2, int y2)
{
    if (!isConnectedToServer()) {
        qDebug() << "[TCP] Failed to send coordinates, no connection.";
        emit errorOccurred("Not connected to server");
        return false;
    }

    QJsonObject message = createBaseMessage("send_coordinates");
    QJsonObject coordinates;
    coordinates["x1"] = x1;
    coordinates["y1"] = y1;
    coordinates["x2"] = x2;
    coordinates["y2"] = y2;

    message["coordinates"] = coordinates;
    message["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    bool success = sendJsonMessage(message);
    if (success) {
        qDebug() << "[TCP] Coordinates sent successfully:" << x1 << y1 << x2 << y2;
    } else {
        qDebug() << "[TCP] Failed to send coordinates.";
    }

    return success;
}

void TcpCommunicator::setVideoView(VideoGraphicsView* videoView)
{
    m_videoView = videoView;
}

bool TcpCommunicator::sendDetectionLine(const DetectionLineData &lineData)
{
    if (!isConnectedToServer()) {
        qDebug() << "[TCP] Failed to send detection line, no connection.";
        emit errorOccurred("Not connected to server");
        return false;
    }

    QJsonObject message;
    message["request_id"] = 2;

    QJsonObject data;
    data["index"] = lineData.index;
    data["x1"] = lineData.x1;
    data["x2"] = lineData.x2;
    data["y1"] = lineData.y1;
    data["y2"] = lineData.y2;
    data["name"] = lineData.name;
    data["mode"] = lineData.mode;

    message["data"] = data;

    bool success = sendJsonMessage(message);
    if (success) {
        qDebug() << "[TCP] Detection line sent successfully - index:" << lineData.index;
    } else {
        qDebug() << "[TCP] Failed to send detection line.";
    }

    return success;
}

bool TcpCommunicator::sendRoadLine(const RoadLineData &lineData)
{
    if (!isConnectedToServer()) {
        qDebug() << "[TCP] Failed to send road line, no connection.";
        emit errorOccurred("Not connected to server");
        return false;
    }

    QJsonObject message;
    message["request_id"] = 5;

    QJsonObject data;
    data["index"] = lineData.index;
    data["matrixNum1"] = lineData.matrixNum1;
    data["x1"] = lineData.x1;
    data["y1"] = lineData.y1;
    data["matrixNum2"] = lineData.matrixNum2;
    data["x2"] = lineData.x2;
    data["y2"] = lineData.y2;

    message["data"] = data;

    bool success = sendJsonMessage(message);
    if (success) {
        qDebug() << "[TCP] Road line sent successfully - index:" << lineData.index;
    } else {
        qDebug() << "[TCP] Failed to send road line.";
    }

    return success;
}

bool TcpCommunicator::sendPerpendicularLine(const PerpendicularLineData &lineData)
{
    if (!isConnectedToServer()) {
        qDebug() << "[TCP] 연결이 없어 수직선 전송 실패";
        emit errorOccurred("서버에 연결되지 않음");
        return false;
    }

    QJsonObject message;
    message["request_id"] = 6;

    QJsonObject data;
    data["index"] = lineData.index;
    data["a"] = lineData.a;
    data["b"] = lineData.b;

    message["data"] = data;

    bool success = sendJsonMessage(message);
    if (success) {
        qDebug() << "[TCP] 수직선 전송 성공 - index:" << lineData.index;
    } else {
        qDebug() << "[TCP] 수직선 전송 실패.";
    }

    return success;
}

// 저장된 도로선 데이터 요청 함수 (request_id: 7)
bool TcpCommunicator::requestSavedRoadLines()
{
    if (!isConnectedToServer()) {
        qDebug() << "[TCP] 연결이 없어 저장된 도로선 데이터 요청 실패";
        emit errorOccurred("서버에 연결되지 않음");
        return false;
    }

    // 서버에 저장된 도로선 데이터 요청 (request_id: 7)
    QJsonObject message;
    message["request_id"] = 7;  // 도로선 select all 요청

    bool success = sendJsonMessage(message);
    if (success) {
        qDebug() << "[TCP] 저장된 도로선 데이터 요청 성공 (request_id: 7)";
    } else {
        qDebug() << "[TCP] 저장된 도로선 데이터 요청 실패";
    }

    return success;
}

// 저장된 감지선 데이터 요청 함수 (request_id: 3)
bool TcpCommunicator::requestSavedDetectionLines()
{
    if (!isConnectedToServer()) {
        qDebug() << "[TCP] 연결이 없어 저장된 감지선 데이터 요청 실패";
        emit errorOccurred("서버에 연결되지 않음");
        return false;
    }

    // 서버에 저장된 감지선 데이터 요청 (request_id: 3)
    QJsonObject message;
    message["request_id"] = 3;  // 감지선 select all 요청

    bool success = sendJsonMessage(message);
    if (success) {
        qDebug() << "[TCP] 저장된 감지선 데이터 요청 전송 성공 (request_id: 3)";
    } else {
        qDebug() << "[TCP] 저장된 감지선 데이터 요청 전송 실패";
    }

    return success;
}

bool TcpCommunicator::requestDeleteLines()
{
    if (!isConnectedToServer()) {
        qDebug() << "[TCP] 연결이 없어 저장된 선 데이터 삭제 실패";
        emit errorOccurred("서버에 연결되지 않음");
        return false;
    }

    // 서버에 저장된 감지선 데이터 요청 (request_id: 4)
    QJsonObject message;
    message["request_id"] = 4;  // 감지선, 기준선, 수직선 delete all 요청

    bool success = sendJsonMessage(message);
    if (success) {
        qDebug() << "[TCP] 저장된 선 데이터 삭제 전송 성공 (request_id: 4)";
    } else {
        qDebug() << "[TCP] 저장된 선 데이터 삭제 전송 실패";
    }

    return success;
}

bool TcpCommunicator::sendMultipleRoadLines(const QList<RoadLineData> &roadLines)
{
    if (!isConnectedToServer()) {
        qDebug() << "[TCP] Failed to send multiple road lines, no connection.";
        emit errorOccurred("Not connected to server");
        return false;
    }

    bool allSuccess = true;
    int successCount = 0;

    for (const auto &line : roadLines) {
        if (sendRoadLine(line)) {
            successCount++;
        } else {
            allSuccess = false;
        }
        QThread::msleep(100);
    }

    qDebug() << "[TCP] Multiple road lines sending complete - Success:" << successCount
             << "/ Total:" << roadLines.size();

    return allSuccess;
}

bool TcpCommunicator::sendMultipleDetectionLines(const QList<DetectionLineData> &detectionLines)
{
    if (!isConnectedToServer()) {
        qDebug() << "[TCP] Failed to send multiple detection lines, no connection.";
        emit errorOccurred("Not connected to server");
        return false;
    }

    bool allSuccess = true;
    int successCount = 0;

    for (const auto &line : detectionLines) {
        if (sendDetectionLine(line)) {
            successCount++;
        } else {
            allSuccess = false;
        }
        QThread::msleep(50);
    }

    qDebug() << "[TCP] Multiple detection lines sending complete - Success:" << successCount
             << "/ Total:" << detectionLines.size();

    return allSuccess;
}

// Modified sendCategorizedLineCoordinates function
bool TcpCommunicator::sendCategorizedLineCoordinates(const QList<CategorizedLineData> &roadLines, const QList<CategorizedLineData> &detectionLines)
{
    if (!isConnectedToServer()) {
        qDebug() << "[TCP] Failed to send categorized coordinates, no connection.";
        emit errorOccurred("Not connected to server");
        return false;
    }

    bool roadSuccess = true;
    bool detectionSuccess = true;

    // Convert and send road lines according to server format
    if (!roadLines.isEmpty()) {
        QList<RoadLineData> serverRoadLines;
        for (int i = 0; i < roadLines.size(); ++i) {
            const auto &line = roadLines[i];

            RoadLineData roadLineData;
            roadLineData.index = i + 1;  // 1부터 시작하는 인덱스
            roadLineData.matrixNum1 = (i % 4) + 1;  // 시작점 Matrix (1-4 순환)
            roadLineData.x1 = line.x1;
            roadLineData.y1 = line.y1;
            roadLineData.matrixNum2 = ((i + 1) % 4) + 1;  // 끝점 Matrix (다른 번호)
            roadLineData.x2 = line.x2;
            roadLineData.y2 = line.y2;

            serverRoadLines.append(roadLineData);
        }

        roadSuccess = sendMultipleRoadLines(serverRoadLines);
    }

    // Convert and send detection lines according to server format
    if (!detectionLines.isEmpty()) {
        QList<DetectionLineData> serverDetectionLines;
        for (int i = 0; i < detectionLines.size(); ++i) {
            const auto &line = detectionLines[i];

            DetectionLineData detectionLineData;
            detectionLineData.index = i + 1;  // Index starting from 1
            detectionLineData.x1 = line.x1;
            detectionLineData.y1 = line.y1;
            detectionLineData.x2 = line.x2;
            detectionLineData.y2 = line.y2;
            detectionLineData.name = QString("DetectionLine%1").arg(i + 1);
            detectionLineData.mode = "BothDirections";  // Default: both directions
            detectionLineData.leftMatrixNum = 1;   // Default value
            detectionLineData.rightMatrixNum = 2;  // Default value

            serverDetectionLines.append(detectionLineData);
        }

        detectionSuccess = sendMultipleDetectionLines(serverDetectionLines);
    }

    bool overallSuccess = roadSuccess && detectionSuccess;

    if (overallSuccess) {
        qDebug() << "[TCP] Categorized coordinates sent successfully - Road lines:" << roadLines.size()
        << "items, Detection lines:" << detectionLines.size() << "items";
        emit categorizedCoordinatesConfirmed(true, "Coordinates sent successfully", roadLines.size(), detectionLines.size());
    } else {
        qDebug() << "[TCP] Failed to send categorized coordinates.";
        emit categorizedCoordinatesConfirmed(false, "Failed to send coordinates", 0, 0);
    }

    return overallSuccess;
}

void TcpCommunicator::requestImageData(const QString &date, int hour)
{
    if (!isConnectedToServer()) {
        qDebug() << "[TCP] Failed to request image data, no connection.";
        emit errorOccurred("Not connected to server");
        return;
    }

    QJsonObject message;
    message["request_id"] = 1;

    QJsonObject data;
    QString requestDate = date.isEmpty() ? QDate::currentDate().toString("yyyy-MM-dd") : date;

    if (hour >= 0 && hour <= 23) {
        data["start_timestamp"] = QString("%1T%2").arg(requestDate).arg(hour, 2, 10, QChar('0'));
        data["end_timestamp"] = QString("%1T%2").arg(requestDate).arg(hour + 1, 2, 10, QChar('0'));
    } else {
        data["start_timestamp"] = requestDate + "T00";
        data["end_timestamp"] = requestDate + "T23";
    }

    message["data"] = data;

    bool success = sendJsonMessage(message);
    if (success) {
        qDebug() << "[TCP] Image request sent - request_id: 1, Date:" << requestDate << "Hour:" << hour;
        emit statusUpdated("Requesting images...");
    } else {
        qDebug() << "[TCP] Failed to request image data.";
        emit errorOccurred("Failed to send image request");
    }
}

void TcpCommunicator::setConnectionTimeout(int timeoutMs)
{
    m_connectionTimeoutMs = timeoutMs;
}

void TcpCommunicator::setReconnectEnabled(bool enabled)
{
    m_reconnectEnabled = enabled;
}

void TcpCommunicator::onConnected()
{
    m_connectionTimer->stop();
    m_isConnected = true;
    m_reconnectAttempts = 0;

    qDebug() << "[TCP] Server connection successful.";
    emit connected();
    emit statusUpdated("Connected to server");
}

void TcpCommunicator::onDisconnected()
{
    m_isConnected = false;
    qDebug() << "[TCP] Disconnected from server.";

    // Add log for socket state
    qDebug() << "[TCP] Socket state:" << m_socket->state();
    qDebug() << "[TCP] Socket error:" << m_socket->errorString();

    emit disconnected();
    emit statusUpdated("Disconnected from server");

    // Attempt reconnection only on unexpected disconnections
    if (m_reconnectEnabled && m_reconnectAttempts < m_maxReconnectAttempts) {
        qDebug() << "[TCP] Scheduling reconnection attempt...";
        m_reconnectTimer->setInterval(m_reconnectDelayMs);
        m_reconnectTimer->start();
    }
}

void TcpCommunicator::onReadyRead()
{
    static QByteArray buffer;
    static quint32 expectedLength = 0;
    static bool lengthReceived = false;

    // Read all available data from the socket
    QByteArray newData = m_socket->readAll();
    buffer.append(newData);

    qDebug() << "[TCP] Data received:" << newData.size() << "bytes, Total buffer size:" << buffer.size();

    while (true) {
        // Step 1: Read message length (4 bytes)
        if (!lengthReceived) {
            if (buffer.size() < 4) {
                // Length information has not fully arrived
                break;
            }

            // Extract length information
            QDataStream lengthStream(buffer.left(4));
            lengthStream.setByteOrder(QDataStream::BigEndian);
            lengthStream >> expectedLength;

            buffer.remove(0, 4); // Remove length information
            lengthReceived = true;

            qDebug() << "[TCP] Message length received:" << expectedLength << "bytes";
        }

        // Step 2: Read the actual message data
        if (lengthReceived) {
            if (buffer.size() < expectedLength) {
                // The message has not fully arrived
                qDebug() << "[TCP] Waiting for message... Current:" << buffer.size() << "/ Required:" << expectedLength;
                break;
            }

            // Extract the complete message
            QByteArray messageData = buffer.left(expectedLength);
            buffer.remove(0, expectedLength);

            // Reset state
            lengthReceived = false;
            expectedLength = 0;

            qDebug() << "[TCP] Complete message received:" << messageData.size() << "bytes";

            // JSON parsing and processing
            QString messageString = QString::fromUtf8(messageData);
            QJsonParseError error;
            QJsonDocument doc = QJsonDocument::fromJson(messageData, &error);

            if (error.error == QJsonParseError::NoError && doc.isObject()) {
                QJsonObject jsonObj = doc.object();
                logJsonMessage(jsonObj, false);
                processJsonMessage(jsonObj);
            } else {
                qDebug() << "[TCP] JSON parsing error:" << error.errorString();
                qDebug() << "[TCP] Original message:" << messageString.left(200) << "...";
                emit messageReceived(messageString);
            }
        }
    }
}

void TcpCommunicator::onError(QAbstractSocket::SocketError error)
{
    m_connectionTimer->stop();
    m_isConnected = false;

    QString errorString;
    switch (error) {
    case QAbstractSocket::ConnectionRefusedError:
        errorString = "Connection refused. Please check if the server is running.";
        break;
    case QAbstractSocket::RemoteHostClosedError:
        errorString = "The remote host closed the connection.";
        break;
    case QAbstractSocket::HostNotFoundError:
        errorString = "Host not found. Please check the IP address.";
        break;
    case QAbstractSocket::SocketTimeoutError:
        errorString = "Connection timed out.";
        break;
    case QAbstractSocket::NetworkError:
        errorString = "A network error occurred.";
        break;
    default:
        errorString = QString("Socket error: %1").arg(m_socket->errorString());
        break;
    }

    qDebug() << "[TCP] Socket error:" << errorString;
    emit errorOccurred(errorString);

    if (m_reconnectEnabled && m_reconnectAttempts < m_maxReconnectAttempts) {
        m_reconnectTimer->setInterval(m_reconnectDelayMs);
        m_reconnectTimer->start();
    }
}

void TcpCommunicator::onConnectionTimeout()
{
    qDebug() << "[TCP] Connection timeout.";
    m_socket->abort();
    emit errorOccurred("Connection timed out.");
}

void TcpCommunicator::attemptReconnection()
{
    if (m_reconnectAttempts >= m_maxReconnectAttempts) {
        qDebug() << "[TCP] Maximum reconnection attempts exceeded.";
        emit errorOccurred("Exceeded maximum reconnection attempts.");
        return;
    }

    m_reconnectAttempts++;
    qDebug() << "[TCP] Reconnection attempt" << m_reconnectAttempts << "/" << m_maxReconnectAttempts;

    emit statusUpdated(QString("Reconnecting... (%1/%2)").arg(m_reconnectAttempts).arg(m_maxReconnectAttempts));

    m_connectionTimer->start();
    m_socket->connectToHostEncrypted(m_host, static_cast<qint16>(m_port));
}

void TcpCommunicator::onSslEncrypted() {
    qDebug() << "[TCP] SSL encrypted connection established.";
}

void TcpCommunicator::onSslErrors(const QList<QSslError> &errors) {
    qDebug() << "[TCP] SSL 오류 발생 - 총" << errors.size() << "개의 오류";
    for (const auto &err : errors) {
        qDebug() << "[TCP] SSL Error:" << err.errorString();
    }
    
    // 개발/테스트 환경에서는 SSL 오류를 무시하여 연결 진행
    // 프로덕션 환경에서는 적절한 인증서를 설정해야 함
    qDebug() << "[TCP] SSL 오류 무시하고 연결 계속 진행";
    m_socket->ignoreSslErrors();
}

void TcpCommunicator::onSocketConnected()
{
    qDebug() << "[TCP] 소켓 연결 성공";

    m_isConnected = true;
    m_reconnectAttempts = 0;
    stopReconnectTimer();

    emit connected();
}

void TcpCommunicator::onSocketDisconnected()
{
    qDebug() << "[TCP] 소켓 연결 해제";

    m_isConnected = false;
    emit disconnected();

    // 자동 재연결 시도
    if (m_autoReconnect && !m_host.isEmpty() && m_port > 0) {
        startReconnectTimer();
    }
}

void TcpCommunicator::onSocketReadyRead()
{
    while (m_socket->canReadLine()) {
        QByteArray data = m_socket->readLine();
        QString message = QString::fromUtf8(data).trimmed();

        if (!message.isEmpty()) {
            qDebug() << "[TCP] 메시지 수신:" << message;

            QJsonParseError error;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

            if (error.error == QJsonParseError::NoError && jsonDoc.isObject()) {
                processJsonMessage(jsonDoc.object());
            } else {
                qDebug() << "[TCP] JSON 파싱 오류:" << error.errorString();
                emit messageReceived(message);
            }
        }
    }
}

void TcpCommunicator::onSocketError(QAbstractSocket::SocketError error)
{
    QString errorString = m_socket->errorString();
    qDebug() << "[TCP] 소켓 오류:" << error << "-" << errorString;

    m_isConnected = false;
    emit errorOccurred(errorString);

    // 자동 재연결 시도 (연결 오류인 경우)
    if (m_autoReconnect && !m_host.isEmpty() && m_port > 0) {
        startReconnectTimer();
    }
}

void TcpCommunicator::onReconnectTimer()
{
    if (m_reconnectAttempts >= m_maxReconnectAttempts) {
        qDebug() << "[TCP] Maximum reconnection attempts exceeded.";
        emit errorOccurred("Exceeded maximum reconnection attempts.");
        return;
    }

    m_reconnectAttempts++;
    qDebug() << "[TCP] Reconnection attempt" << m_reconnectAttempts << "/" << m_maxReconnectAttempts;

    emit statusUpdated(QString("Reconnecting... (%1/%2)").arg(m_reconnectAttempts).arg(m_maxReconnectAttempts));

    m_connectionTimer->start();
    m_socket->connectToHostEncrypted(m_host, static_cast<qint16>(m_port));
}

void TcpCommunicator::startReconnectTimer()
{
    if (!m_reconnectTimer->isActive() && m_reconnectAttempts < m_maxReconnectAttempts) {
        qDebug() << "[TCP] 재연결 타이머 시작 (" << m_reconnectDelayMs << "ms 후)";
        m_reconnectTimer->start();
    }
}

void TcpCommunicator::stopReconnectTimer()
{
    if (m_reconnectTimer->isActive()) {
        qDebug() << "[TCP] 재연결 타이머 중지";
        m_reconnectTimer->stop();
    }
}

void TcpCommunicator::processJsonMessage(const QJsonObject &jsonObj)
{
    // request_id 또는 response_id 확인 (서버 호환성)
    int requestId = jsonObj["request_id"].toInt();
    if (requestId == 0) {
        requestId = jsonObj["response_id"].toInt();
    }

    qDebug() << "[TCP] JSON 메시지 처리 - request_id/response_id:" << requestId;

    // 기타 응답 처리
    switch (requestId) {
    case 10: // 이미지 요청 응답
        handleImagesResponse(jsonObj);
        break;
    case 12:
        handleSavedDetectionLinesResponse(jsonObj);
        handleDetectionLinesFromServer(jsonObj);
        break;
    case 16:
        handleSavedRoadLinesResponse(jsonObj);
        handleRoadLinesFromServer(jsonObj);
        break;
    case 200: // BBox 데이터 응답
        handleBBoxResponse(jsonObj);
        break;
    default:
        qDebug() << "[TCP] 알 수 없는 request_id:" << requestId;
        QJsonDocument doc(jsonObj);
        emit messageReceived(doc.toJson(QJsonDocument::Compact));
        break;
    }
}

// request_id 12: 감지선 데이터 처리 핸들러
void TcpCommunicator::handleDetectionLinesFromServer(const QJsonObject &jsonObj)
{
    qDebug() << "[TCP] handleDetectionLinesFromServer 호출됨 (request_id: 12)";
    QList<DetectionLineData> detectionLines;

    if (jsonObj.contains("data") && jsonObj["data"].isArray()) {
        QJsonArray dataArray = jsonObj["data"].toArray();
        for (int i = 0; i < dataArray.size(); ++i) {
            QJsonObject detectionLineObj = dataArray[i].toObject();
            DetectionLineData detectionLine;
            detectionLine.index = detectionLineObj["index"].toInt();
            detectionLine.x1 = detectionLineObj["x1"].toInt();
            detectionLine.y1 = detectionLineObj["y1"].toInt();
            detectionLine.x2 = detectionLineObj["x2"].toInt();
            detectionLine.y2 = detectionLineObj["y2"].toInt();
            detectionLine.name = detectionLineObj["name"].toString();
            detectionLine.mode = detectionLineObj["mode"].toString();
            // detectionLine.leftMatrixNum = detectionLineObj["leftMatrixNum"].toInt();
            // detectionLine.rightMatrixNum = detectionLineObj["rightMatrixNum"].toInt();
            detectionLines.append(detectionLine);
        }
    }

    // VideoGraphicsView 인스턴스에 감지선 데이터 전달
    if (m_videoView) {
        m_videoView->loadSavedDetectionLines(detectionLines);
    } else {
        qDebug() << "[TCP] m_videoView가 nullptr입니다. 감지선 데이터를 전달할 수 없습니다.";
    }
}

void TcpCommunicator::handleRoadLinesFromServer(const QJsonObject &jsonObj)
{
    qDebug() << "[TCP] handleRoadLinesFromServer 호출됨 (request_id: 16)";
    QList<RoadLineData> roadLines;

    if (jsonObj.contains("data") && jsonObj["data"].isArray()) {
        QJsonArray dataArray = jsonObj["data"].toArray();
        for (int i = 0; i < dataArray.size(); ++i) {
            QJsonObject roadLineObj = dataArray[i].toObject();
            RoadLineData roadLine;
            roadLine.index = roadLineObj["index"].toInt();
            roadLine.x1 = roadLineObj["x1"].toInt();
            roadLine.y1 = roadLineObj["y1"].toInt();
            roadLine.x2 = roadLineObj["x2"].toInt();
            roadLine.y2 = roadLineObj["y2"].toInt();
            roadLine.matrixNum1 = roadLineObj["matrixNum1"].toInt();
            roadLine.matrixNum2 = roadLineObj["matrixNum2"].toInt();
            roadLines.append(roadLine);
        }
    }

    // VideoGraphicsView 인스턴스에 감지선 데이터 전달
    if (m_videoView) {
        m_videoView->loadSavedRoadLines(roadLines);
    } else {
        qDebug() << "[TCP] m_videoView가 nullptr입니다. 도로기준선 데이터를 전달할 수 없습니다.";
    }
}

QString TcpCommunicator::saveBase64Image(const QString &base64Data, const QString &timestamp)
{
    QString cleanBase64 = base64Data;
    if (cleanBase64.contains(",")) {
        cleanBase64 = cleanBase64.split(",").last();
    }

    QByteArray imageData = QByteArray::fromBase64(cleanBase64.toUtf8());

    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString cleanTimestamp = timestamp;
    cleanTimestamp.replace(":", "_").replace("-", "_");
    QString fileName = QString("CCTVImage%1.jpg").arg(cleanTimestamp);
    QString filePath = QDir(tempDir).absoluteFilePath(fileName);

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(imageData);
        file.close();
        qDebug() << "[TCP] Base64 image saved successfully:" << filePath;
        return filePath;
    } else {
        qDebug() << "[TCP] Failed to save Base64 image:" << filePath;
        return QString();
    }
}

void TcpCommunicator::handleImagesResponse(const QJsonObject &jsonObj)
{
    qDebug() << "[TCP] Processing image response...";

    if (!jsonObj.contains("data")) {
        qDebug() << "[TCP] 'data' field not found in response.";
        emit errorOccurred("The 'data' field is missing in the server response.");
        return;
    }

    QJsonArray dataArray = jsonObj["data"].toArray();
    qDebug() << "[TCP] Size of data array:" << dataArray.size();

    QList<ImageData> images;

    for (int i = 0; i < dataArray.size(); ++i) {
        QJsonValue value = dataArray[i];
        if (!value.isObject()) {
            qDebug() << "[TCP] data[" << i << "] is not an object.";
            continue;
        }

        QJsonObject imageObj = value.toObject();

        if (!imageObj.contains("image") || !imageObj.contains("timestamp")) {
            qDebug() << "[TCP] Image object[" << i << "] is missing required fields.";
            continue;
        }

        ImageData imageData;
        QString base64Image = imageObj["image"].toString();
        imageData.timestamp = imageObj["timestamp"].toString();

        imageData.imagePath = saveBase64Image(base64Image, imageData.timestamp);
        imageData.logText = QString("Detection time: %1").arg(imageData.timestamp);
        imageData.detectionType = "vehicle";
        imageData.direction = "unknown";

        if (!imageData.imagePath.isEmpty()) {
            images.append(imageData);
        }
    }

    qDebug() << "[TCP] Number of parsed images:" << images.size();
    emit imagesReceived(images);
    emit statusUpdated(QString("Loaded %1 images.").arg(images.size()));
}

void TcpCommunicator::handleCoordinatesResponse(const QJsonObject &jsonObj)
{
    bool success = jsonObj["success"].toBool();
    QString message = jsonObj["message"].toString();

    qDebug() << "[TCP] Coordinates response - Success:" << success << "Message:" << message;
    emit coordinatesConfirmed(success, message);

    if (success) {
        emit statusUpdated("Coordinates sent successfully");
    } else {
        emit errorOccurred("Failed to send coordinates: " + message);
    }
}

void TcpCommunicator::handleDetectionLineResponse(const QJsonObject &jsonObj)
{
    bool success = true;
    QString message = "Detection line setup complete";

    if (jsonObj.contains("success")) {
        success = jsonObj["success"].toBool();
    }

    if (jsonObj.contains("message")) {
        message = jsonObj["message"].toString();
    }

    // Extract additional information from the data field
    if (jsonObj.contains("data") && jsonObj["data"].isObject()) {
        QJsonObject data = jsonObj["data"].toObject();
        int index = data["index"].toInt();
        QString name = data["name"].toString();
        QString mode = data["mode"].toString();

        qDebug() << "[TCP] Detection line response - index:" << index
                 << "name:" << name << "mode:" << mode << "Success:" << success;

        message = QString("Detection line '%1' (index: %2, mode: %3) setup %4")
                      .arg(name).arg(index).arg(mode).arg(success ? "succeeded" : "failed");
    }

    emit detectionLineConfirmed(success, message);

    if (success) {
        emit statusUpdated("Detection line setup complete");
    } else {
        emit errorOccurred("Failed to set up detection line: " + message);
    }
}

void TcpCommunicator::handleRoadLineResponse(const QJsonObject &jsonObj)
{
    bool success = true;
    QString message = "Road line setup complete";

    if (jsonObj.contains("success")) {
        success = jsonObj["success"].toBool();
    }

    if (jsonObj.contains("message")) {
        message = jsonObj["message"].toString();
    }

    if (jsonObj.contains("data") && jsonObj["data"].isObject()) {
        QJsonObject data = jsonObj["data"].toObject();
        int index = data["index"].toInt();
        int matrixNum1 = data["matrixNum1"].toInt();
        int x1 = data["x1"].toInt();
        int y1 = data["y1"].toInt();
        int matrixNum2 = data["matrixNum2"].toInt();
        int x2 = data["x2"].toInt();
        int y2 = data["y2"].toInt();

        qDebug() << "[TCP] Road line response - index:" << index
                 << "start:(" << x1 << "," << y1 << ") matrix:" << matrixNum1
                 << "end:(" << x2 << "," << y2 << ") matrix:" << matrixNum2
                 << "Success:" << success;

        message = QString("Road line #%1 (start:(%2,%3) Matrix:%4, end:(%5,%6) Matrix:%7) setup %8")
                      .arg(index).arg(x1).arg(y1).arg(matrixNum1)
                      .arg(x2).arg(y2).arg(matrixNum2)
                      .arg(success ? "succeeded" : "failed");
    }

    emit roadLineConfirmed(success, message);

    if (success) {
        emit statusUpdated("Road line setup complete");
    } else {
        emit errorOccurred("Failed to set up road line: " + message);
    }
}

void TcpCommunicator::handlePerpendicularLineResponse(const QJsonObject &jsonObj)
{
    bool success = true;
    QString message = "수직선 설정 완료";

    if (jsonObj.contains("success")) {
        success = jsonObj["success"].toBool();
    }

    if (jsonObj.contains("message")) {
        message = jsonObj["message"].toString();
    }

    // data 필드에서 추가 정보 추출
    if (jsonObj.contains("data") && jsonObj["data"].isObject()) {
        QJsonObject data = jsonObj["data"].toObject();
        int index = data["index"].toInt();
        double a = data["a"].toDouble();
        double b = data["b"].toDouble();

        qDebug() << "[TCP] 수직선 응답 - index:" << index
                 << "y = " << a << "x + " << b << "성공:" << success;

        message = QString("수직선 (index: %1, y = %2x + %3) 설정 %4")
                      .arg(index).arg(a).arg(b).arg(success ? "성공" : "실패");
    }

    emit perpendicularLineConfirmed(success, message);

    if (success) {
        emit statusUpdated("수직선 설정 완료");
    } else {
        emit errorOccurred("수직선 설정 실패: " + message);
    }
}

// 저장된 도로선 데이터 응답 처리 함수 (request_id: 7)
void TcpCommunicator::handleSavedRoadLinesResponse(const QJsonObject &jsonObj)
{
    qDebug() << "[TCP] 저장된 도로선 데이터 응답 처리 중... (request_id: 7)";
    qDebug() << "[TCP] Full JSON:" << QJsonDocument(jsonObj).toJson(QJsonDocument::Compact);

    m_receivedRoadLines.clear();

    if (jsonObj.contains("data") && jsonObj["data"].isArray()) {
        QJsonArray dataArray = jsonObj["data"].toArray();

        for (int i = 0; i < dataArray.size(); ++i) {
            QJsonObject roadLineObj = dataArray[i].toObject();

            RoadLineData roadLine;
            roadLine.index = roadLineObj["index"].toInt();
            roadLine.matrixNum1 = roadLineObj["matrixNum1"].toInt();
            roadLine.x1 = roadLineObj["x1"].toInt();
            roadLine.y1 = roadLineObj["y1"].toInt();
            roadLine.matrixNum2 = roadLineObj["matrixNum2"].toInt();
            roadLine.x2 = roadLineObj["x2"].toInt();
            roadLine.y2 = roadLineObj["y2"].toInt();

            m_receivedRoadLines.append(roadLine);

            qDebug() << "[TCP] 도로선 로드됨 - index:" << roadLine.index
                     << "start:(" << roadLine.x1 << "," << roadLine.y1 << ") matrix:" << roadLine.matrixNum1
                     << "end:(" << roadLine.x2 << "," << roadLine.y2 << ") matrix:" << roadLine.matrixNum2;
        }
    }

    qDebug() << "[TCP] 저장된 도로선 데이터 로드 완료 - 도로선:" << m_receivedRoadLines.size() << "개";

    m_roadLinesReceived = true;
    emit savedRoadLinesReceived(m_receivedRoadLines);

}

// 저장된 감지선 데이터 응답 처리 함수 (request_id: 3)
void TcpCommunicator::handleSavedDetectionLinesResponse(const QJsonObject &jsonObj)
{
    qDebug() << "[TCP] 저장된 감지선 데이터 응답 처리 중... (request_id: 3)";
    qDebug() << "[TCP] Full JSON:" << QJsonDocument(jsonObj).toJson(QJsonDocument::Compact);

    m_receivedDetectionLines.clear();

    if (jsonObj.contains("data") && jsonObj["data"].isArray()) {
        QJsonArray dataArray = jsonObj["data"].toArray();

        for (int i = 0; i < dataArray.size(); ++i) {
            QJsonObject detectionLineObj = dataArray[i].toObject();

            DetectionLineData detectionLine;
            detectionLine.index = detectionLineObj["index"].toInt();
            detectionLine.x1 = detectionLineObj["x1"].toInt();
            detectionLine.y1 = detectionLineObj["y1"].toInt();
            detectionLine.x2 = detectionLineObj["x2"].toInt();
            detectionLine.y2 = detectionLineObj["y2"].toInt();
            detectionLine.name = detectionLineObj["name"].toString();
            detectionLine.mode = detectionLineObj["mode"].toString();

            m_receivedDetectionLines.append(detectionLine);

            qDebug() << "[TCP] 감지선 로드됨 - index:" << detectionLine.index
                     << "name:" << detectionLine.name << "mode:" << detectionLine.mode
                     << "좌표:(" << detectionLine.x1 << "," << detectionLine.y1 << ") → ("
                     << detectionLine.x2 << "," << detectionLine.y2 << ")";
        }
    }

    qDebug() << "[TCP] 저장된 감지선 데이터 로드 완료 - 감지선:" << m_receivedDetectionLines.size() << "개";

    m_detectionLinesReceived = true;
    emit savedDetectionLinesReceived(m_receivedDetectionLines);

}

void TcpCommunicator::handleCategorizedCoordinatesResponse(const QJsonObject &jsonObj)
{
    bool success = jsonObj["success"].toBool();
    QString message = jsonObj["message"].toString();

    qDebug() << "[TCP] Categorized coordinates response - Success:" << success << "Message:" << message;

    if (jsonObj.contains("data") && jsonObj["data"].isObject()) {
        QJsonObject data = jsonObj["data"].toObject();
        int roadLinesProcessed = data["road_lines_processed"].toInt();
        int detectionLinesProcessed = data["detection_lines_processed"].toInt();
        int totalProcessed = data["total_processed"].toInt();

        qDebug() << "[TCP] Processed coordinates - Road lines:" << roadLinesProcessed << "items, Detection lines:" << detectionLinesProcessed << "items, Total:" << totalProcessed << "items";

        emit categorizedCoordinatesConfirmed(success, message, roadLinesProcessed, detectionLinesProcessed);
    } else {
        emit categorizedCoordinatesConfirmed(success, message, 0, 0);
    }

    if (success) {
        emit statusUpdated("Categorized coordinates sent successfully");
    } else {
        emit errorOccurred("Failed to send categorized coordinates: " + message);
    }
}

void TcpCommunicator::handleStatusUpdate(const QJsonObject &jsonObj)
{
    QString status = jsonObj["status"].toString();
    QString message = jsonObj["message"].toString();

    qDebug() << "[TCP] Status update - Status:" << status << "Message:" << message;
    emit statusUpdated(message.isEmpty() ? status : message);
}

void TcpCommunicator::handleErrorResponse(const QJsonObject &jsonObj)
{
    QString errorMsg = jsonObj["message"].toString("Unknown error");
    QString errorCode = jsonObj["error_code"].toString();

    qDebug() << "[TCP] Server error - Code:" << errorCode << "Message:" << errorMsg;
    emit errorOccurred(errorMsg);
}

QJsonObject TcpCommunicator::createBaseMessage(const QString &type) const
{
    QJsonObject message;
    message["type"] = type;
    message["client_id"] = "qt_cctv_client";
    message["version"] = "1.0";
    return message;
}

QString TcpCommunicator::messageTypeToString(MessageType type) const
{
    switch (type) {
    case MessageType::REQUEST_IMAGES: return "request_images";
    case MessageType::IMAGES_RESPONSE: return "images_response";
    case MessageType::SEND_COORDINATES: return "send_coordinates";
    case MessageType::COORDINATES_RESPONSE: return "coordinates_response";
    case MessageType::STATUS_UPDATE: return "status_update";
    case MessageType::ERROR_RESPONSE: return "error_response";
    default: return "unknown";
    }
}

MessageType TcpCommunicator::stringToMessageType(const QString &typeStr) const
{
    if (typeStr == "request_images") return MessageType::REQUEST_IMAGES;
    if (typeStr == "images_response") return MessageType::IMAGES_RESPONSE;
    if (typeStr == "send_coordinates") return MessageType::SEND_COORDINATES;
    if (typeStr == "coordinates_response") return MessageType::COORDINATES_RESPONSE;
    if (typeStr == "status_update") return MessageType::STATUS_UPDATE;
    if (typeStr == "error_response") return MessageType::ERROR_RESPONSE;
    return MessageType::ERROR_RESPONSE; // Default
}

void TcpCommunicator::logJsonMessage(const QJsonObject &jsonObj, bool outgoing) const
{
    QString direction = outgoing ? "Sent" : "Received";
    int requestId = jsonObj["request_id"].toInt();

    qDebug() << QString("[TCP] JSON %1 - request_id: %2").arg(direction).arg(requestId);

// Only print full JSON in debug mode
#ifdef QT_DEBUG
    QJsonDocument doc(jsonObj);
    qDebug() << "JSON Content:" << doc.toJson(QJsonDocument::Compact);
#endif
}

void TcpCommunicator::setupSocket()
{
    // 소켓 설정이 필요한 경우 여기에 구현
}

// BBox 데이터 처리 함수
void TcpCommunicator::handleBBoxResponse(const QJsonObject &jsonObj)
{
    qDebug() << "[TCP] handleBBoxResponse 호출됨 (response_id: 200)";
    
    QList<BBox> bboxes;
    qint64 timestamp = QDateTime::currentMSecsSinceEpoch();
    
    if (jsonObj.contains("bboxes") && jsonObj["bboxes"].isArray()) {
        QJsonArray bboxArray = jsonObj["bboxes"].toArray();
        
        for (int i = 0; i < bboxArray.size(); ++i) {
            QJsonObject bboxObj = bboxArray[i].toObject();
            
            BBox bbox;
            bbox.object_id = bboxObj["id"].toInt();
            bbox.type = bboxObj["type"].toString();
            bbox.confidence = bboxObj["confidence"].toDouble();
            bbox.rect = QRect(
                bboxObj["x"].toInt(),
                bboxObj["y"].toInt(),
                bboxObj["width"].toInt(),
                bboxObj["height"].toInt()
            );
            
            bboxes.append(bbox);
        }
    }
    
    // 타임스탬프가 JSON에 포함되어 있다면 사용
    if (jsonObj.contains("timestamp")) {
        timestamp = jsonObj["timestamp"].toVariant().toLongLong();
    }
    
    qDebug() << QString("[TCP] BBox 데이터 파싱 완료 - 총 %1개 객체").arg(bboxes.size());
    
    // BBox 데이터를 시그널로 전달
    emit bboxesReceived(bboxes, timestamp);
}
