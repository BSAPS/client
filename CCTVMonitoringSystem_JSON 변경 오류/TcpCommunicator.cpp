#include "TcpCommunicator.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>

TcpCommunicator::TcpCommunicator(QObject *parent)
    : QObject(parent)
    , m_socket(nullptr)
    , m_connectionTimer(nullptr)
    , m_reconnectTimer(nullptr)
    , m_port(0)
    , m_isConnected(false)
    , m_connectionTimeoutMs(15000)  // 15초
    , m_reconnectEnabled(true)
    , m_reconnectAttempts(0)
    , m_maxReconnectAttempts(5)
    , m_reconnectDelayMs(3000)      // 3초
{
    m_socket = new QTcpSocket(this);

    // 소켓 시그널 연결
    connect(m_socket, &QTcpSocket::connected, this, &TcpCommunicator::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &TcpCommunicator::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &TcpCommunicator::onReadyRead);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
            this, &TcpCommunicator::onError);

    // 연결 타임아웃 타이머
    m_connectionTimer = new QTimer(this);
    m_connectionTimer->setSingleShot(true);
    m_connectionTimer->setInterval(m_connectionTimeoutMs);
    connect(m_connectionTimer, &QTimer::timeout, this, &TcpCommunicator::onConnectionTimeout);

    // 재연결 타이머
    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout, this, &TcpCommunicator::attemptReconnection);
}

TcpCommunicator::~TcpCommunicator()
{
    disconnectFromServer();
}

void TcpCommunicator::connectToServer(const QString &host, int port)
{
    if (m_isConnected) {
        disconnectFromServer();
    }

    m_host = host;
    m_port = port;
    m_reconnectAttempts = 0;

    qDebug() << "[TCP] 서버 연결 시도:" << host << ":" << port;

    m_connectionTimer->start();
    m_socket->connectToHost(host, static_cast<quint16>(port));
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
        qDebug() << "[TCP] 연결이 없어 좌표 전송 실패";
        emit errorOccurred("서버에 연결되지 않음");
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
        qDebug() << "[TCP] 좌표 전송 성공:" << x1 << y1 << x2 << y2;
    } else {
        qDebug() << "[TCP] 좌표 전송 실패";
    }

    return success;
}

void TcpCommunicator::requestImageData(const QString &date, int hour)
{
    if (!isConnectedToServer()) {
        qDebug() << "[TCP] 연결이 없어 이미지 요청 실패";
        emit errorOccurred("서버에 연결되지 않음");
        return;
    }

    QJsonObject message = createBaseMessage("request_images");

    // 날짜가 지정되지 않으면 오늘 날짜 사용
    QString requestDate = date.isEmpty() ? QDate::currentDate().toString("yyyy-MM-dd") : date;
    message["date"] = requestDate;

    // 시간이 지정되지 않으면 전체 시간 요청
    if (hour >= 0 && hour <= 23) {
        message["hour"] = hour;
    }

    message["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    bool success = sendJsonMessage(message);
    if (success) {
        qDebug() << "[TCP] 이미지 요청 전송 완료 - 날짜:" << requestDate << "시간:" << hour;
        emit statusUpdated("이미지 요청 중...");
    } else {
        qDebug() << "[TCP] 이미지 요청 실패";
        emit errorOccurred("이미지 요청 전송 실패");
    }
}

bool TcpCommunicator::sendJsonMessage(const QJsonObject &message)
{
    if (!isConnectedToServer()) {
        return false;
    }

    QJsonDocument doc(message);
    QByteArray data = doc.toJson(QJsonDocument::Compact) + "\n";

    logJsonMessage(message, true);

    qint64 bytesWritten = m_socket->write(data);
    if (bytesWritten == -1) {
        qDebug() << "[TCP] 메시지 전송 실패:" << m_socket->errorString();
        return false;
    }

    m_socket->flush();
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

    qDebug() << "[TCP] 서버 연결 성공";
    emit connectionStatusChanged(true);
    emit statusUpdated("서버 연결됨");
}

void TcpCommunicator::onDisconnected()
{
    m_isConnected = false;
    qDebug() << "[TCP] 서버 연결 해제";
    emit connectionStatusChanged(false);
    emit statusUpdated("서버 연결 해제됨");

    // 재연결 시도
    if (m_reconnectEnabled && m_reconnectAttempts < m_maxReconnectAttempts) {
        m_reconnectTimer->setInterval(m_reconnectDelayMs);
        m_reconnectTimer->start();
    }
}

void TcpCommunicator::onReadyRead()
{
    QByteArray data = m_socket->readAll();
    m_receivedData.append(QString::fromUtf8(data));

    // 완전한 JSON 메시지가 수신될 때까지 대기 (줄바꿈으로 구분)
    while (m_receivedData.contains('\n')) {
        int index = m_receivedData.indexOf('\n');
        QString message = m_receivedData.left(index).trimmed();
        m_receivedData = m_receivedData.mid(index + 1);

        if (!message.isEmpty()) {
            // JSON 파싱 시도
            QJsonParseError error;
            QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8(), &error);

            if (error.error == QJsonParseError::NoError && doc.isObject()) {
                QJsonObject jsonObj = doc.object();
                logJsonMessage(jsonObj, false);
                processJsonMessage(jsonObj);
            } else {
                qDebug() << "[TCP] JSON 파싱 오류:" << error.errorString();
                qDebug() << "[TCP] 원본 메시지:" << message;
                emit errorOccurred("잘못된 JSON 형식: " + error.errorString());
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
        errorString = "연결이 거부되었습니다. 서버가 실행 중인지 확인하세요.";
        break;
    case QAbstractSocket::RemoteHostClosedError:
        errorString = "서버가 연결을 종료했습니다.";
        break;
    case QAbstractSocket::HostNotFoundError:
        errorString = "호스트를 찾을 수 없습니다. IP 주소를 확인하세요.";
        break;
    case QAbstractSocket::SocketTimeoutError:
        errorString = "연결 시간이 초과되었습니다.";
        break;
    case QAbstractSocket::NetworkError:
        errorString = "네트워크 오류가 발생했습니다.";
        break;
    default:
        errorString = QString("소켓 오류: %1").arg(m_socket->errorString());
        break;
    }

    qDebug() << "[TCP] 소켓 오류:" << errorString;
    emit errorOccurred(errorString);

    // 재연결 시도
    if (m_reconnectEnabled && m_reconnectAttempts < m_maxReconnectAttempts) {
        m_reconnectTimer->setInterval(m_reconnectDelayMs);
        m_reconnectTimer->start();
    }
}

void TcpCommunicator::onConnectionTimeout()
{
    qDebug() << "[TCP] 연결 타임아웃";
    m_socket->abort();
    emit errorOccurred("연결 시간이 초과되었습니다.");
}

void TcpCommunicator::attemptReconnection()
{
    if (m_reconnectAttempts >= m_maxReconnectAttempts) {
        qDebug() << "[TCP] 최대 재연결 시도 횟수 초과";
        emit errorOccurred("최대 재연결 시도 횟수를 초과했습니다.");
        return;
    }

    m_reconnectAttempts++;
    qDebug() << "[TCP] 재연결 시도" << m_reconnectAttempts << "/" << m_maxReconnectAttempts;

    emit statusUpdated(QString("재연결 시도 중... (%1/%2)").arg(m_reconnectAttempts).arg(m_maxReconnectAttempts));

    m_connectionTimer->start();
    m_socket->connectToHost(m_host, static_cast<quint16>(m_port));
}

void TcpCommunicator::processJsonMessage(const QJsonObject &jsonObj)
{
    QString type = jsonObj["type"].toString();
    MessageType msgType = stringToMessageType(type);

    switch (msgType) {
    case MessageType::IMAGES_RESPONSE:
        handleImagesResponse(jsonObj);
        break;
    case MessageType::COORDINATES_RESPONSE:
        handleCoordinatesResponse(jsonObj);
        break;
    case MessageType::STATUS_UPDATE:
        handleStatusUpdate(jsonObj);
        break;
    case MessageType::ERROR_RESPONSE:
        handleErrorResponse(jsonObj);
        break;
    default:
        qDebug() << "[TCP] 알 수 없는 메시지 타입:" << type;
        break;
    }

    emit messageReceived(QJsonDocument(jsonObj).toJson(QJsonDocument::Compact));
}

void TcpCommunicator::handleImagesResponse(const QJsonObject &jsonObj)
{
    qDebug() << "[TCP] 이미지 응답 처리 중...";

    bool success = jsonObj["success"].toBool();
    if (!success) {
        QString errorMsg = jsonObj["message"].toString("이미지 요청 실패");
        qDebug() << "[TCP] 이미지 요청 실패:" << errorMsg;
        emit errorOccurred(errorMsg);
        return;
    }

    QJsonArray imagesArray = jsonObj["images"].toArray();
    QList<ImageData> images;

    for (const QJsonValue &value : imagesArray) {
        QJsonObject imageObj = value.toObject();

        ImageData imageData;
        imageData.imagePath = imageObj["path"].toString();
        imageData.timestamp = imageObj["timestamp"].toString();
        imageData.logText = imageObj["log"].toString();
        imageData.detectionType = imageObj["detection_type"].toString();
        imageData.direction = imageObj["direction"].toString();

        // 이미지 파일 존재 확인
        if (!imageData.imagePath.isEmpty()) {
            QFileInfo fileInfo(imageData.imagePath);
            if (fileInfo.exists()) {
                images.append(imageData);
            } else {
                qDebug() << "[TCP] 이미지 파일이 존재하지 않음:" << imageData.imagePath;
            }
        }
    }

    qDebug() << "[TCP] 파싱된 이미지 개수:" << images.size();
    emit imagesReceived(images);
    emit statusUpdated(QString("이미지 %1개를 불러왔습니다.").arg(images.size()));
}

void TcpCommunicator::handleCoordinatesResponse(const QJsonObject &jsonObj)
{
    bool success = jsonObj["success"].toBool();
    QString message = jsonObj["message"].toString();

    qDebug() << "[TCP] 좌표 응답 - 성공:" << success << "메시지:" << message;
    emit coordinatesConfirmed(success, message);

    if (success) {
        emit statusUpdated("좌표 전송 완료");
    } else {
        emit errorOccurred("좌표 전송 실패: " + message);
    }
}

void TcpCommunicator::handleStatusUpdate(const QJsonObject &jsonObj)
{
    QString status = jsonObj["status"].toString();
    QString message = jsonObj["message"].toString();

    qDebug() << "[TCP] 상태 업데이트 - 상태:" << status << "메시지:" << message;
    emit statusUpdated(message.isEmpty() ? status : message);
}

void TcpCommunicator::handleErrorResponse(const QJsonObject &jsonObj)
{
    QString errorMsg = jsonObj["message"].toString("알 수 없는 오류");
    QString errorCode = jsonObj["error_code"].toString();

    qDebug() << "[TCP] 서버 오류 - 코드:" << errorCode << "메시지:" << errorMsg;
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
    return MessageType::ERROR_RESPONSE; // 기본값
}

void TcpCommunicator::logJsonMessage(const QJsonObject &jsonObj, bool outgoing) const
{
    QString direction = outgoing ? "송신" : "수신";
    QString type = jsonObj["type"].toString();

    qDebug() << QString("[TCP] JSON %1 - 타입: %2").arg(direction, type);

// 디버그 모드에서만 전체 JSON 출력
#ifdef QT_DEBUG
    QJsonDocument doc(jsonObj);
    qDebug() << "JSON 내용:" << doc.toJson(QJsonDocument::Compact);
#endif
}
