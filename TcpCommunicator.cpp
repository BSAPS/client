#include "TcpCommunicator.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QSslSocket>
#include <QSslError>
#include <QSslConfiguration>

TcpCommunicator::TcpCommunicator(QObject *parent)
    : QObject(parent)
    , m_socket(nullptr)
    , m_connectionTimer(nullptr)
    , m_reconnectTimer(nullptr)
    , m_port(0)
    , m_isConnected(false)
    , m_connectionTimeoutMs(30000)  // 30 seconds
    , m_reconnectEnabled(true)
    , m_reconnectAttempts(0)
    , m_maxReconnectAttempts(3)
    , m_reconnectDelayMs(5000)      // 5 seconds
{
    m_socket = new QSslSocket(this);
    setupSslConfiguration();

    // Keep-Alive settings
    m_socket->setSocketOption(QAbstractSocket::KeepAliveOption, 1);
    m_socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);

    // Connect socket signals
    connect(m_socket, &QTcpSocket::connected, this, &TcpCommunicator::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &TcpCommunicator::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &TcpCommunicator::onReadyRead);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
            this, &TcpCommunicator::onError);

    // Connect SSL signals
    connect(m_socket, &QSslSocket::encrypted, this, &TcpCommunicator::onSslEncrypted);
    connect(m_socket, QOverload<const QList<QSslError>&>::of(&QSslSocket::sslErrors),
            this, &TcpCommunicator::onSslErrors);

    // Connection timeout timer
    m_connectionTimer = new QTimer(this);
    m_connectionTimer->setSingleShot(true);
    m_connectionTimer->setInterval(m_connectionTimeoutMs);
    connect(m_connectionTimer, &QTimer::timeout, this, &TcpCommunicator::onConnectionTimeout);

    // Reconnection timer
    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout, this, &TcpCommunicator::attemptReconnection);
}

TcpCommunicator::~TcpCommunicator()
{
    disconnectFromServer();
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
        qDebug() << "[TCP] Warning: ca-cert.pem file not found or could not be read.";
    }

    // 4. Apply SSL configuration to the socket
    m_socket->setSslConfiguration(sslConfiguration);

    // 5. Set server certificate verification mode
    m_socket->setPeerVerifyMode(QSslSocket::VerifyPeer);
}

void TcpCommunicator::connectToServer(const QString &host, int port)
{
    if (m_isConnected) {
        disconnectFromServer();
    }
    m_host = host;
    m_port = port;
    m_reconnectAttempts = 0;
    qDebug() << "[TCP] Attempting to connect to server:" << host << ":" << port;
    m_connectionTimer->start();
    m_socket->connectToHostEncrypted(host, static_cast<quint16>(port));
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

bool TcpCommunicator::isConnectedToServer() const
{
    return m_isConnected && m_socket && m_socket->state() == QAbstractSocket::ConnectedState;
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

bool TcpCommunicator::sendDetectionLine(const DetectionLineData &lineData)
{
    if (!isConnectedToServer()) {
        qDebug() << "[TCP] Failed to send detection line, no connection.";
        emit errorOccurred("Not connected to server");
        return false;
    }

    // Create JSON message according to server format
    QJsonObject message;
    message["request_id"] = 2;  // request_id for detection line defined by server

    QJsonObject data;
    data["index"] = lineData.index;
    data["x1"] = lineData.x1;
    data["x2"] = lineData.x2;
    data["y1"] = lineData.y1;
    data["y2"] = lineData.y2;
    data["name"] = lineData.name;
    data["mode"] = lineData.mode;  // "Right", "Left", "BothDirections"
    data["leftMatrixNum"] = lineData.leftMatrixNum;
    data["rightMatrixNum"] = lineData.rightMatrixNum;

    message["data"] = data;

    bool success = sendJsonMessage(message);
    if (success) {
        qDebug() << "[TCP] Detection line sent successfully - index:" << lineData.index
                 << "name:" << lineData.name << "mode:" << lineData.mode;
    } else {
        qDebug() << "[TCP] Failed to send detection line.";
    }

    return success;
}

// Added sendRoadLine function (after sendDetectionLine function)
bool TcpCommunicator::sendRoadLine(const RoadLineData &lineData)
{
    if (!isConnectedToServer()) {
        qDebug() << "[TCP] Failed to send road line, no connection.";
        emit errorOccurred("Not connected to server");
        return false;
    }

    // Create JSON message according to server format
    QJsonObject message;
    message["request_id"] = 5;  // request_id for road line defined by server

    QJsonObject data;
    data["matrixNum"] = lineData.matrixNum;
    data["x1"] = lineData.x1;  // 시작점 x
    data["y1"] = lineData.y1;  // 시작점 y
    data["x2"] = lineData.x2;  // 끝점 x
    data["y2"] = lineData.y2;  // 끝점 y

    message["data"] = data;

    bool success = sendJsonMessage(message);
    if (success) {
        qDebug() << "[TCP] Road line sent successfully - matrixNum:" << lineData.matrixNum
                 << "start:(" << lineData.x1 << "," << lineData.y1 << ")"
                 << "end:(" << lineData.x2 << "," << lineData.y2 << ")";
    } else {
        qDebug() << "[TCP] Failed to send road line.";
    }

    return success;
}

// sendPerpendicularLine 함수 추가 (sendMultipleRoadLines 함수 다음에)
bool TcpCommunicator::sendPerpendicularLine(const PerpendicularLineData &lineData)
{
    if (!isConnectedToServer()) {
        qDebug() << "[TCP] 연결이 없어 수직선 전송 실패";
        emit errorOccurred("서버에 연결되지 않음");
        return false;
    }

    // 서버 양식에 맞춘 JSON 메시지 생성 (y = ax + b 형태)
    QJsonObject message;
    message["request_id"] = 6;  // 서버에서 정의한 수직선 request_id

    QJsonObject data;
    data["index"] = lineData.index;
    data["a"] = lineData.a;  // y = ax + b에서 a값 (기울기)
    data["b"] = lineData.b;  // y = ax + b에서 b값 (y절편)

    message["data"] = data;

    bool success = sendJsonMessage(message);
    if (success) {
        qDebug() << "[TCP] 수직선 전송 성공 - index:" << lineData.index
                 << "y = " << lineData.a << "x + " << lineData.b;
    } else {
        qDebug() << "[TCP] 수직선 전송 실패.";
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

        // Short delay between each transmission (considering server processing time)
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

        // Short delay between each transmission (considering server processing time)
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
            roadLineData.matrixNum = (i % 4) + 1;  // Cycle 1-4
            roadLineData.x1 = line.x1;
            roadLineData.x2 = line.x2;

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
            detectionLineData.name = QString("detection_line_%1").arg(i + 1);
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

    // JSON format from client to server (request_id: 1)
    QJsonObject message;
    message["request_id"] = 1;  // Client -> Server request ID

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

bool TcpCommunicator::sendJsonMessage(const QJsonObject &message)
{
    if (!isConnectedToServer()) {
        qDebug() << "[TCP] Message sending failed, not connected.";
        return false;
    }

    QJsonDocument doc(message);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

    // Log the JSON content to be sent
    qDebug() << "[TCP] JSON data to send:" << QString::fromUtf8(jsonData);

    // 1. Send the message length as 4 bytes first
    quint32 messageLength = static_cast<quint32>(jsonData.size());
    QByteArray lengthData;
    QDataStream lengthStream(&lengthData, QIODevice::WriteOnly);
    lengthStream.setByteOrder(QDataStream::BigEndian); // Network byte order
    lengthStream << messageLength;

    logJsonMessage(message, true);

    // 2. Send length information
    qint64 lengthWritten = m_socket->write(lengthData);
    if (lengthWritten != 4) {
        qDebug() << "[TCP] Failed to send length info - bytes written:" << lengthWritten;
        return false;
    }

    // 3. Send the actual JSON data
    qint64 dataWritten = m_socket->write(jsonData);
    if (dataWritten != jsonData.size()) {
        qDebug() << "[TCP] Failed to send JSON data - Expected:" << jsonData.size() << "Actual:" << dataWritten;
        qDebug() << "[TCP] Socket error:" << m_socket->errorString();
        return false;
    }

    // Wait until the data is actually sent
    bool flushed = m_socket->flush();
    if (!flushed) {
        qDebug() << "[TCP] Data flush failed.";
    }

    qDebug() << "[TCP] Message sent successfully - Length:" << messageLength << "bytes, Flush:" << (flushed ? "Success" : "Fail");
    return true;
}

void TcpCommunicator::setConnectionTimeout(int timeoutMs)
{
    m_connectionTimeoutMs = timeoutMs;
    m_connectionTimer->setInterval(timeoutMs);
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

    // Attempt reconnection
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
    m_socket->connectToHostEncrypted(m_host, static_cast<quint16>(m_port));
}

void TcpCommunicator::onSslEncrypted() {
    qDebug() << "[TCP] SSL encrypted connection established.";
}

void TcpCommunicator::onSslErrors(const QList<QSslError> &errors) {
    for (const auto &err : errors) {
        qDebug() << "[TCP] SSL Error:" << err.errorString();
    }
    // In a real service, comment out the line below (for testing purposes only)
    // m_socket->ignoreSslErrors();
}

// Added handling for request_id 5 in processJsonMessage function
void TcpCommunicator::processJsonMessage(const QJsonObject &jsonObj)
{
    int requestId = jsonObj["request_id"].toInt();

    qDebug() << "[TCP] Received request_id:" << requestId;
    qDebug() << "[TCP] JSON keys:" << jsonObj.keys();

    // Process based on the server response's request_id
    if (requestId == 10) {
        // Server -> Client image response (request_id: 10)
        qDebug() << "[TCP] Image response received (request_id: 10)";

        // Check if data array exists
        if (jsonObj.contains("data") && jsonObj["data"].isArray()) {
            handleImagesResponse(jsonObj);
        } else {
            qDebug() << "[TCP] 'data' array not found in image response.";
            emit errorOccurred("Server response format error: 'data' array is missing.");
        }
    } else if (requestId == 2) {
        // Detection line response
        handleDetectionLineResponse(jsonObj);
    } else if (requestId == 5) {
        // Road line response
        handleRoadLineResponse(jsonObj);
    } else if (requestId == 4) {
        // Categorized coordinates response (legacy)
        handleCategorizedCoordinatesResponse(jsonObj);
    } else {
        qDebug() << "[TCP] Unknown request_id:" << requestId;
        qDebug() << "[TCP] Full JSON:" << QJsonDocument(jsonObj).toJson(QJsonDocument::Compact);
        // Process as a generic message
        emit messageReceived(QJsonDocument(jsonObj).toJson(QJsonDocument::Compact));
    }
}

QString TcpCommunicator::saveBase64Image(const QString &base64Data, const QString &timestamp)
{
    // Remove header from Base64 data (e.g., data:image/jpeg;base64,)
    QString cleanBase64 = base64Data;
    if (cleanBase64.contains(",")) {
        cleanBase64 = cleanBase64.split(",").last();
    }

    // Decode Base64
    QByteArray imageData = QByteArray::fromBase64(cleanBase64.toUtf8());

    // Create temporary file path
    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString cleanTimestamp = timestamp;
    cleanTimestamp.replace(":", "_").replace("-", "_");
    QString fileName = QString("cctv_image_%1.jpg").arg(cleanTimestamp);
    QString filePath = QDir(tempDir).absoluteFilePath(fileName);

    // Save to file
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
    qDebug() << "[TCP] Full JSON:" << QJsonDocument(jsonObj).toJson(QJsonDocument::Compact);

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
        qDebug() << "[TCP] Image object[" << i << "] keys:" << imageObj.keys();

        if (!imageObj.contains("image") || !imageObj.contains("timestamp")) {
            qDebug() << "[TCP] Image object[" << i << "] is missing required fields.";
            continue;
        }

        ImageData imageData;
        QString base64Image = imageObj["image"].toString();
        imageData.timestamp = imageObj["timestamp"].toString();

        qDebug() << "[TCP] Processing image - timestamp:" << imageData.timestamp;
        qDebug() << "[TCP] Base64 data length:" << base64Image.length();

        // Save Base64 image to a file
        imageData.imagePath = saveBase64Image(base64Image, imageData.timestamp);
        imageData.logText = QString("Detection time: %1").arg(imageData.timestamp);
        imageData.detectionType = "vehicle";
        imageData.direction = "unknown";

        if (!imageData.imagePath.isEmpty()) {
            images.append(imageData);
            qDebug() << "[TCP] Image saved successfully:" << imageData.imagePath;
        } else {
            qDebug() << "[TCP] Failed to save image.";
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
    // Process response for detection line settings from the server
    bool success = true;  // Default value
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

// Added handleRoadLineResponse function (after handleDetectionLineResponse function)
void TcpCommunicator::handleRoadLineResponse(const QJsonObject &jsonObj)
{
    // Process response for road line settings from the server
    bool success = true;  // Default value
    QString message = "Road line setup complete";

    if (jsonObj.contains("success")) {
        success = jsonObj["success"].toBool();
    }

    if (jsonObj.contains("message")) {
        message = jsonObj["message"].toString();
    }

    // Extract additional information from the data field
    if (jsonObj.contains("data") && jsonObj["data"].isObject()) {
        QJsonObject data = jsonObj["data"].toObject();
        int matrixNum = data["matrixNum"].toInt();
        int x1 = data["x1"].toInt();
        int y1 = data["y1"].toInt();
        int x2 = data["x2"].toInt();
        int y2 = data["y2"].toInt();

        qDebug() << "[TCP] Road line response - matrixNum:" << matrixNum
                 << "start:(" << x1 << "," << y1 << ")"
                 << "end:(" << x2 << "," << y2 << ")"
                 << "Success:" << success;

        message = QString("Road line (Matrix: %1, start:(%2,%3), end:(%4,%5)) setup %6")
                      .arg(matrixNum).arg(x1).arg(y1).arg(x2).arg(y2)
                      .arg(success ? "succeeded" : "failed");
    }

    emit roadLineConfirmed(success, message);

    if (success) {
        emit statusUpdated("Road line setup complete");
    } else {
        emit errorOccurred("Failed to set up road line: " + message);
    }
}


// handlePerpendicularLineResponse 함수 추가 (handleRoadLineResponse 함수 다음에)
void TcpCommunicator::handlePerpendicularLineResponse(const QJsonObject &jsonObj)
{
    // 서버에서 수직선 설정에 대한 응답 처리
    bool success = true;  // 기본값
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
