#include "TcpCommunicator.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QThread>

TcpCommunicator::TcpCommunicator(QObject *parent)
    : QObject(parent)
    , m_socket(new QTcpSocket(this))
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
{
    qDebug() << "[TCP] TcpCommunicator 생성자 호출";

    // TCP 소켓 시그널 연결
    connect(m_socket, &QTcpSocket::connected, this, &TcpCommunicator::onSocketConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &TcpCommunicator::onSocketDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &TcpCommunicator::onSocketReadyRead);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
            this, &TcpCommunicator::onSocketError);

    // 재연결 타이머 설정
    m_reconnectTimer->setSingleShot(true);
    m_reconnectTimer->setInterval(m_reconnectDelayMs);
    connect(m_reconnectTimer, &QTimer::timeout, this, &TcpCommunicator::onReconnectTimer);

    qDebug() << "[TCP] TcpCommunicator 초기화 완료";
}

TcpCommunicator::~TcpCommunicator()
{
    qDebug() << "[TCP] TcpCommunicator 소멸자 호출";

    m_autoReconnect = false;

    if (m_reconnectTimer && m_reconnectTimer->isActive()) {
        m_reconnectTimer->stop();
    }

    if (m_socket && m_socket->state() == QTcpSocket::ConnectedState) {
        m_socket->disconnectFromHost();
        if (!m_socket->waitForDisconnected(3000)) {
            m_socket->abort();
        }
    }
}

void TcpCommunicator::connectToServer(const QString &host, quint16 port)
{
    qDebug() << "[TCP] connectToServer 호출 - 호스트:" << host << "포트:" << port;

    m_host = host;
    m_port = port;

    // 이미 연결되어 있으면 연결 해제 후 재연결
    if (m_socket->state() != QTcpSocket::UnconnectedState) {
        qDebug() << "[TCP] 기존 연결 해제 중... 현재 상태:" << m_socket->state();
        m_socket->disconnectFromHost();
        if (m_socket->state() != QTcpSocket::UnconnectedState) {
            m_socket->waitForDisconnected(1000);
        }
    }

    qDebug() << "[TCP] 서버 연결 시도:" << host << ":" << port;
    m_socket->connectToHost(host, port);

    // 연결 타임아웃 설정 (10초)
    if (!m_socket->waitForConnected(10000)) {
        qDebug() << "[TCP] 연결 타임아웃 또는 실패:" << m_socket->errorString();
        emit errorOccurred("연결 타임아웃: " + m_socket->errorString());
    }
}

void TcpCommunicator::disconnectFromServer()
{
    qDebug() << "[TCP] disconnectFromServer 호출";

    m_autoReconnect = false;
    stopReconnectTimer();

    if (m_socket && m_socket->state() == QTcpSocket::ConnectedState) {
        qDebug() << "[TCP] 서버 연결 해제 중...";
        m_socket->disconnectFromHost();
        if (!m_socket->waitForDisconnected(3000)) {
            m_socket->abort();
        }
    }
}

bool TcpCommunicator::isConnectedToServer() const
{
    bool connected = m_isConnected && m_socket && m_socket->state() == QTcpSocket::ConnectedState;
    return connected;
}

bool TcpCommunicator::sendJsonMessage(const QJsonObject &message)
{
    if (!isConnectedToServer()) {
        qDebug() << "[TCP] 메시지 전송 실패 - 서버에 연결되지 않음";
        return false;
    }

    QJsonDocument doc(message);
    QByteArray data = doc.toJson(QJsonDocument::Compact);

    qDebug() << "[TCP] JSON 메시지 전송 시도:" << data;

    // 메시지 끝에 개행 문자 추가 (서버에서 라인 단위로 읽을 수 있도록)
    data.append('\n');

    qint64 bytesWritten = m_socket->write(data);
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
    data["leftMatrixNum"] = lineData.leftMatrixNum;
    data["rightMatrixNum"] = lineData.rightMatrixNum;

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

bool TcpCommunicator::requestSavedRoadLines()
{
    if (!isConnectedToServer()) {
        qDebug() << "[TCP] 연결이 없어 저장된 도로선 데이터 요청 실패";
        emit errorOccurred("서버에 연결되지 않음");
        return false;
    }

    QJsonObject message;
    message["request_id"] = 7;

    bool success = sendJsonMessage(message);
    if (success) {
        qDebug() << "[TCP] 저장된 도로선 데이터 요청 성공 (request_id: 7)";
    } else {
        qDebug() << "[TCP] 저장된 도로선 데이터 요청 실패";
    }

    return success;
}

bool TcpCommunicator::requestSavedDetectionLines()
{
    if (!isConnectedToServer()) {
        qDebug() << "[TCP] 연결이 없어 저장된 감지선 데이터 요청 실패";
        emit errorOccurred("서버에 연결되지 않음");
        return false;
    }

    QJsonObject message;
    message["request_id"] = 3;

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

    QJsonObject message;
    message["request_id"] = 4;

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

void TcpCommunicator::setVideoView(VideoGraphicsView *videoView)
{
    m_videoView = videoView;
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
    if (m_autoReconnect && !m_host.isEmpty() && m_port > 0 && m_reconnectAttempts < m_maxReconnectAttempts) {
        m_reconnectAttempts++;
        qDebug() << "[TCP] 재연결 시도" << m_reconnectAttempts << "/" << m_maxReconnectAttempts;
        connectToServer(m_host, m_port);
    } else if (m_reconnectAttempts >= m_maxReconnectAttempts) {
        qDebug() << "[TCP] 최대 재연결 시도 횟수 초과";
        emit errorOccurred("최대 재연결 시도 횟수를 초과했습니다.");
    }
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
    int requestId = jsonObj["request_id"].toInt();

    qDebug() << "[TCP] JSON 메시지 처리 - request_id:" << requestId;

    // 로그인 관련 응답 처리 (request_id: 8-12)
    if (requestId >= 8 && requestId <= 12) {
        // LoginWindow에서 처리하도록 원본 JSON 문자열 전달
        QJsonDocument doc(jsonObj);
        QString jsonString = doc.toJson(QJsonDocument::Compact);
        emit messageReceived(jsonString);
        return;
    }

    // 기타 응답 처리
    switch (requestId) {
    case 1: // 이미지 요청 응답
        handleImagesResponse(jsonObj);
        break;
    case 2: // 감지선 응답
        handleDetectionLineResponse(jsonObj);
        break;
    case 3: // 저장된 감지선 데이터 응답
        handleSavedDetectionLinesResponse(jsonObj);
        break;
    case 4: // 선 삭제 응답
        handleCoordinatesResponse(jsonObj);
        break;
    case 5: // 도로선 응답
        handleRoadLineResponse(jsonObj);
        break;
    case 6: // 수직선 응답
        handlePerpendicularLineResponse(jsonObj);
        break;
    case 7: // 저장된 도로선 데이터 응답
        handleSavedRoadLinesResponse(jsonObj);
        break;
    default:
        qDebug() << "[TCP] 알 수 없는 request_id:" << requestId;
        QJsonDocument doc(jsonObj);
        emit messageReceived(doc.toJson(QJsonDocument::Compact));
        break;
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

    emit perpendicularLineConfirmed(success, message);

    if (success) {
        emit statusUpdated("수직선 설정 완료");
    } else {
        emit errorOccurred("수직선 설정 실패: " + message);
    }
}

void TcpCommunicator::handleSavedRoadLinesResponse(const QJsonObject &jsonObj)
{
    qDebug() << "[TCP] 저장된 도로선 데이터 응답 처리 중... (request_id: 7)";

    QList<RoadLineData> roadLines;

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

            roadLines.append(roadLine);
        }
    }

    qDebug() << "[TCP] 저장된 도로선 데이터 로드 완료 - 도로선:" << roadLines.size() << "개";
    emit savedRoadLinesReceived(roadLines);
}

void TcpCommunicator::handleSavedDetectionLinesResponse(const QJsonObject &jsonObj)
{
    qDebug() << "[TCP] 저장된 감지선 데이터 응답 처리 중... (request_id: 3)";

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

            detectionLines.append(detectionLine);
        }
    }

    qDebug() << "[TCP] 저장된 감지선 데이터 로드 완료 - 감지선:" << detectionLines.size() << "개";
    emit savedDetectionLinesReceived(detectionLines);
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

void TcpCommunicator::setupSocket()
{
    // 소켓 설정이 필요한 경우 여기에 구현
}
