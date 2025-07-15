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

TcpCommunicator::TcpCommunicator(QObject *parent)
    : QObject(parent)
    , m_socket(nullptr)
    , m_connectionTimer(nullptr)
    , m_reconnectTimer(nullptr)
    , m_port(0)
    , m_isConnected(false)
    , m_connectionTimeoutMs(30000)  // 30초
    , m_reconnectEnabled(true)
    , m_reconnectAttempts(0)
    , m_maxReconnectAttempts(3)
    , m_reconnectDelayMs(5000)      // 5초
{
    m_socket = new QSslSocket(this);
    setupSslConfiguration();

    // Keep-Alive 설정
    m_socket->setSocketOption(QAbstractSocket::KeepAliveOption, 1);
    m_socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);

    // 소켓 시그널 연결
    connect(m_socket, &QTcpSocket::connected, this, &TcpCommunicator::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &TcpCommunicator::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &TcpCommunicator::onReadyRead);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
            this, &TcpCommunicator::onError);

    // SSL 관련 시그널 연결
    connect(m_socket, &QSslSocket::encrypted, this, &TcpCommunicator::onSslEncrypted);
    connect(m_socket, QOverload<const QList<QSslError>&>::of(&QSslSocket::sslErrors),
            this, &TcpCommunicator::onSslErrors);

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

void TcpCommunicator::setupSslConfiguration() {
    // 서버의 CA 인증서 등록 (예: ca-cert.pem 파일)
    QList<QSslCertificate> caCerts = QSslCertificate::fromPath("ca-cert.pem");
    if (!caCerts.isEmpty()) {
        QSslSocket::addDefaultCaCertificates(caCerts);
    }
    // 서버 인증서 검증 활성화 (실서비스)
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
    qDebug() << "[TCP] 서버 연결 시도:" << host << ":" << port;
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

bool TcpCommunicator::sendDetectionLine(const DetectionLineData &lineData)
{
    if (!isConnectedToServer()) {
        qDebug() << "[TCP] 연결이 없어 객체 탐지선 전송 실패";
        emit errorOccurred("서버에 연결되지 않음");
        return false;
    }

    // 서버 양식에 맞춘 JSON 메시지 생성
    QJsonObject message;
    message["request_id"] = 2;  // 서버에서 정의한 객체 탐지선 request_id

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
        qDebug() << "[TCP] 객체 탐지선 전송 성공 - index:" << lineData.index
                 << "name:" << lineData.name << "mode:" << lineData.mode;
    } else {
        qDebug() << "[TCP] 객체 탐지선 전송 실패";
    }

    return success;
}

// sendRoadLine 함수 추가 (sendDetectionLine 함수 다음에)
bool TcpCommunicator::sendRoadLine(const RoadLineData &lineData)
{
    if (!isConnectedToServer()) {
        qDebug() << "[TCP] 연결이 없어 도로 기준선 전송 실패";
        emit errorOccurred("서버에 연결되지 않음");
        return false;
    }

    // 서버 양식에 맞춘 JSON 메시지 생성
    QJsonObject message;
    message["request_id"] = 5;  // 서버에서 정의한 도로 기준선 request_id

    QJsonObject data;
    data["matrixNum"] = lineData.matrixNum;
    data["x1"] = lineData.x1;
    data["x2"] = lineData.x2;

    message["data"] = data;

    bool success = sendJsonMessage(message);
    if (success) {
        qDebug() << "[TCP] 도로 기준선 전송 성공 - matrixNum:" << lineData.matrixNum
                 << "x1:" << lineData.x1 << "x2:" << lineData.x2;
    } else {
        qDebug() << "[TCP] 도로 기준선 전송 실패";
    }

    return success;
}

bool TcpCommunicator::sendMultipleRoadLines(const QList<RoadLineData> &roadLines)
{
    if (!isConnectedToServer()) {
        qDebug() << "[TCP] 연결이 없어 다중 도로 기준선 전송 실패";
        emit errorOccurred("서버에 연결되지 않음");
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

        // 각 전송 사이에 짧은 지연 (서버 처리 시간 고려)
        QThread::msleep(100);
    }

    qDebug() << "[TCP] 다중 도로 기준선 전송 완료 - 성공:" << successCount
             << "/ 총:" << roadLines.size();

    return allSuccess;
}

bool TcpCommunicator::sendMultipleDetectionLines(const QList<DetectionLineData> &detectionLines)
{
    if (!isConnectedToServer()) {
        qDebug() << "[TCP] 연결이 없어 다중 객체 탐지선 전송 실패";
        emit errorOccurred("서버에 연결되지 않음");
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

        // 각 전송 사이에 짧은 지연 (서버 처리 시간 고려)
        QThread::msleep(50);
    }

    qDebug() << "[TCP] 다중 객체 탐지선 전송 완료 - 성공:" << successCount
             << "/ 총:" << detectionLines.size();

    return allSuccess;
}

// sendCategorizedLineCoordinates 함수 수정
bool TcpCommunicator::sendCategorizedLineCoordinates(const QList<CategorizedLineData> &roadLines, const QList<CategorizedLineData> &detectionLines)
{
    if (!isConnectedToServer()) {
        qDebug() << "[TCP] 연결이 없어 카테고리별 좌표 전송 실패";
        emit errorOccurred("서버에 연결되지 않음");
        return false;
    }

    bool roadSuccess = true;
    bool detectionSuccess = true;

    // 도로 기준선을 서버 양식에 맞춰 변환 및 전송
    if (!roadLines.isEmpty()) {
        QList<RoadLineData> serverRoadLines;
        for (int i = 0; i < roadLines.size(); ++i) {
            const auto &line = roadLines[i];

            RoadLineData roadLineData;
            roadLineData.matrixNum = (i % 4) + 1;  // 1-4 순환
            roadLineData.x1 = line.x1;
            roadLineData.x2 = line.x2;

            serverRoadLines.append(roadLineData);
        }

        roadSuccess = sendMultipleRoadLines(serverRoadLines);
    }

    // 객체 탐지선을 서버 양식에 맞춰 변환 및 전송
    if (!detectionLines.isEmpty()) {
        QList<DetectionLineData> serverDetectionLines;
        for (int i = 0; i < detectionLines.size(); ++i) {
            const auto &line = detectionLines[i];

            DetectionLineData detectionLineData;
            detectionLineData.index = i + 1;  // 1부터 시작하는 인덱스
            detectionLineData.x1 = line.x1;
            detectionLineData.y1 = line.y1;
            detectionLineData.x2 = line.x2;
            detectionLineData.y2 = line.y2;
            detectionLineData.name = QString("detection_line_%1").arg(i + 1);
            detectionLineData.mode = "BothDirections";  // 기본값: 양방향
            detectionLineData.leftMatrixNum = 1;   // 기본값
            detectionLineData.rightMatrixNum = 2;  // 기본값

            serverDetectionLines.append(detectionLineData);
        }

        detectionSuccess = sendMultipleDetectionLines(serverDetectionLines);
    }

    bool overallSuccess = roadSuccess && detectionSuccess;

    if (overallSuccess) {
        qDebug() << "[TCP] 카테고리별 좌표 전송 성공 - 도로선:" << roadLines.size()
                 << "개, 탐지선:" << detectionLines.size() << "개";
        emit categorizedCoordinatesConfirmed(true, "좌표 전송 완료", roadLines.size(), detectionLines.size());
    } else {
        qDebug() << "[TCP] 카테고리별 좌표 전송 실패";
        emit categorizedCoordinatesConfirmed(false, "좌표 전송 실패", 0, 0);
    }

    return overallSuccess;
}

void TcpCommunicator::requestImageData(const QString &date, int hour)
{
    if (!isConnectedToServer()) {
        qDebug() << "[TCP] 연결이 없어 이미지 요청 실패";
        emit errorOccurred("서버에 연결되지 않음");
        return;
    }

    // 클라이언트가 서버에게 보내는 JSON 형식 (request_id: 1)
    QJsonObject message;
    message["request_id"] = 1;  // 클라이언트 → 서버 요청 ID

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
        qDebug() << "[TCP] 이미지 요청 전송 완료 - request_id: 1, 날짜:" << requestDate << "시간:" << hour;
        emit statusUpdated("이미지 요청 중...");
    } else {
        qDebug() << "[TCP] 이미지 요청 실패";
        emit errorOccurred("이미지 요청 전송 실패");
    }
}

bool TcpCommunicator::sendJsonMessage(const QJsonObject &message)
{
    if (!isConnectedToServer()) {
        qDebug() << "[TCP] 연결되지 않아 메시지 전송 실패";
        return false;
    }

    QJsonDocument doc(message);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

    // 전송할 JSON 내용을 로그로 출력
    qDebug() << "[TCP] 전송할 JSON 데이터:" << QString::fromUtf8(jsonData);

    // 1. 메시지 길이를 4바이트로 먼저 전송
    quint32 messageLength = static_cast<quint32>(jsonData.size());
    QByteArray lengthData;
    QDataStream lengthStream(&lengthData, QIODevice::WriteOnly);
    lengthStream.setByteOrder(QDataStream::BigEndian); // 네트워크 바이트 순서
    lengthStream << messageLength;

    logJsonMessage(message, true);

    // 2. 길이 정보 전송
    qint64 lengthWritten = m_socket->write(lengthData);
    if (lengthWritten != 4) {
        qDebug() << "[TCP] 길이 정보 전송 실패 - 전송된 바이트:" << lengthWritten;
        return false;
    }

    // 3. 실제 JSON 데이터 전송
    qint64 dataWritten = m_socket->write(jsonData);
    if (dataWritten != jsonData.size()) {
        qDebug() << "[TCP] JSON 데이터 전송 실패 - 예상:" << jsonData.size() << "실제:" << dataWritten;
        qDebug() << "[TCP] 소켓 에러:" << m_socket->errorString();
        return false;
    }

    // 데이터가 실제로 전송될 때까지 대기
    bool flushed = m_socket->flush();
    if (!flushed) {
        qDebug() << "[TCP] 데이터 플러시 실패";
    }

    qDebug() << "[TCP] 메시지 전송 완료 - 길이:" << messageLength << "바이트, 플러시:" << (flushed ? "성공" : "실패");
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
    emit connected();
    emit statusUpdated("서버 연결됨");
}

void TcpCommunicator::onDisconnected()
{
    m_isConnected = false;
    qDebug() << "[TCP] 서버 연결 해제됨";

    // 소켓 상태 확인 로그 추가
    qDebug() << "[TCP] 소켓 상태:" << m_socket->state();
    qDebug() << "[TCP] 소켓 에러:" << m_socket->errorString();

    emit disconnected();
    emit statusUpdated("서버 연결 해제됨");

    // 예상치 못한 연결 해제인 경우에만 재연결 시도
    if (m_reconnectEnabled && m_reconnectAttempts < m_maxReconnectAttempts) {
        qDebug() << "[TCP] 재연결 시도 예약...";
        m_reconnectTimer->setInterval(m_reconnectDelayMs);
        m_reconnectTimer->start();
    }
}

void TcpCommunicator::onReadyRead()
{
    static QByteArray buffer;
    static quint32 expectedLength = 0;
    static bool lengthReceived = false;

    // 소켓에서 사용 가능한 모든 데이터 읽기
    QByteArray newData = m_socket->readAll();
    buffer.append(newData);

    qDebug() << "[TCP] 데이터 수신:" << newData.size() << "바이트, 버퍼 총 크기:" << buffer.size();

    while (true) {
        // 1단계: 메시지 길이 읽기 (4바이트)
        if (!lengthReceived) {
            if (buffer.size() < 4) {
                // 길이 정보가 완전히 도착하지 않음
                break;
            }

            // 길이 정보 추출
            QDataStream lengthStream(buffer.left(4));
            lengthStream.setByteOrder(QDataStream::BigEndian);
            lengthStream >> expectedLength;

            buffer.remove(0, 4); // 길이 정보 제거
            lengthReceived = true;

            qDebug() << "[TCP] 메시지 길이 수신:" << expectedLength << "바이트";
        }

        // 2단계: 실제 메시지 데이터 읽기
        if (lengthReceived) {
            if (buffer.size() < expectedLength) {
                // 메시지가 완전히 도착하지 않음
                qDebug() << "[TCP] 메시지 대기 중... 현재:" << buffer.size() << "/ 필요:" << expectedLength;
                break;
            }

            // 완전한 메시지 추출
            QByteArray messageData = buffer.left(expectedLength);
            buffer.remove(0, expectedLength);

            // 상태 초기화
            lengthReceived = false;
            expectedLength = 0;

            qDebug() << "[TCP] 완전한 메시지 수신:" << messageData.size() << "바이트";

            // JSON 파싱 및 처리
            QString messageString = QString::fromUtf8(messageData);
            QJsonParseError error;
            QJsonDocument doc = QJsonDocument::fromJson(messageData, &error);

            if (error.error == QJsonParseError::NoError && doc.isObject()) {
                QJsonObject jsonObj = doc.object();
                logJsonMessage(jsonObj, false);
                processJsonMessage(jsonObj);
            } else {
                qDebug() << "[TCP] JSON 파싱 오류:" << error.errorString();
                qDebug() << "[TCP] 원본 메시지:" << messageString.left(200) << "...";
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
    m_socket->connectToHostEncrypted(m_host, static_cast<quint16>(m_port));
}

void TcpCommunicator::onSslEncrypted() {
    qDebug() << "[TCP] SSL 암호화 연결 완료";
}

void TcpCommunicator::onSslErrors(const QList<QSslError> &errors) {
    for (const auto &err : errors) {
        qDebug() << "[TCP] SSL 오류:" << err.errorString();
    }
    // 실제 서비스에서는 아래 줄을 주석 처리(테스트용 무시)
    // m_socket->ignoreSslErrors();
}

// processJsonMessage 함수에서 request_id 5 처리 추가
void TcpCommunicator::processJsonMessage(const QJsonObject &jsonObj)
{
    int requestId = jsonObj["request_id"].toInt();

    qDebug() << "[TCP] 수신된 request_id:" << requestId;
    qDebug() << "[TCP] JSON 키들:" << jsonObj.keys();

    // 서버 응답의 request_id에 따라 처리
    if (requestId == 10) {
        // 서버 → 클라이언트 이미지 응답 (request_id: 10)
        qDebug() << "[TCP] 이미지 응답 수신 (request_id: 10)";

        // data 배열이 있는지 확인
        if (jsonObj.contains("data") && jsonObj["data"].isArray()) {
            handleImagesResponse(jsonObj);
        } else {
            qDebug() << "[TCP] 이미지 응답에 data 배열이 없음";
            emit errorOccurred("서버 응답 형식 오류: data 배열이 없음");
        }
    } else if (requestId == 2) {
        // 객체 탐지선 응답
        handleDetectionLineResponse(jsonObj);
    } else if (requestId == 5) {
        // 도로 기준선 응답
        handleRoadLineResponse(jsonObj);
    } else if (requestId == 4) {
        // 카테고리별 좌표 응답 (기존 방식)
        handleCategorizedCoordinatesResponse(jsonObj);
    } else {
        qDebug() << "[TCP] 알 수 없는 request_id:" << requestId;
        qDebug() << "[TCP] 전체 JSON:" << QJsonDocument(jsonObj).toJson(QJsonDocument::Compact);
        // 일반 메시지로 처리
        emit messageReceived(QJsonDocument(jsonObj).toJson(QJsonDocument::Compact));
    }
}

QString TcpCommunicator::saveBase64Image(const QString &base64Data, const QString &timestamp)
{
    // Base64 데이터에서 헤더 제거 (data:image/jpeg;base64, 등)
    QString cleanBase64 = base64Data;
    if (cleanBase64.contains(",")) {
        cleanBase64 = cleanBase64.split(",").last();
    }

    // Base64 디코딩
    QByteArray imageData = QByteArray::fromBase64(cleanBase64.toUtf8());

    // 임시 파일 경로 생성
    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString cleanTimestamp = timestamp;
    cleanTimestamp.replace(":", "_").replace("-", "_");
    QString fileName = QString("cctv_image_%1.jpg").arg(cleanTimestamp);
    QString filePath = QDir(tempDir).absoluteFilePath(fileName);

    // 파일로 저장
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(imageData);
        file.close();
        qDebug() << "[TCP] Base64 이미지 저장 완료:" << filePath;
        return filePath;
    } else {
        qDebug() << "[TCP] Base64 이미지 저장 실패:" << filePath;
        return QString();
    }
}

void TcpCommunicator::handleImagesResponse(const QJsonObject &jsonObj)
{
    qDebug() << "[TCP] 이미지 응답 처리 중...";
    qDebug() << "[TCP] 전체 JSON:" << QJsonDocument(jsonObj).toJson(QJsonDocument::Compact);

    if (!jsonObj.contains("data")) {
        qDebug() << "[TCP] 응답에 data 필드가 없음";
        emit errorOccurred("서버 응답에 data 필드가 없습니다");
        return;
    }

    QJsonArray dataArray = jsonObj["data"].toArray();
    qDebug() << "[TCP] data 배열 크기:" << dataArray.size();

    QList<ImageData> images;

    for (int i = 0; i < dataArray.size(); ++i) {
        QJsonValue value = dataArray[i];
        if (!value.isObject()) {
            qDebug() << "[TCP] data[" << i << "]가 객체가 아님";
            continue;
        }

        QJsonObject imageObj = value.toObject();
        qDebug() << "[TCP] 이미지 객체[" << i << "] 키들:" << imageObj.keys();

        if (!imageObj.contains("image") || !imageObj.contains("timestamp")) {
            qDebug() << "[TCP] 이미지 객체[" << i << "]에 필수 필드가 없음";
            continue;
        }

        ImageData imageData;
        QString base64Image = imageObj["image"].toString();
        imageData.timestamp = imageObj["timestamp"].toString();

        qDebug() << "[TCP] 처리 중인 이미지 - timestamp:" << imageData.timestamp;
        qDebug() << "[TCP] Base64 데이터 길이:" << base64Image.length();

        // Base64 이미지를 파일로 저장
        imageData.imagePath = saveBase64Image(base64Image, imageData.timestamp);
        imageData.logText = QString("감지 시간: %1").arg(imageData.timestamp);
        imageData.detectionType = "vehicle";
        imageData.direction = "unknown";

        if (!imageData.imagePath.isEmpty()) {
            images.append(imageData);
            qDebug() << "[TCP] 이미지 저장 성공:" << imageData.imagePath;
        } else {
            qDebug() << "[TCP] 이미지 저장 실패";
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

void TcpCommunicator::handleDetectionLineResponse(const QJsonObject &jsonObj)
{
    // 서버에서 객체 탐지선 설정에 대한 응답 처리
    bool success = true;  // 기본값
    QString message = "객체 탐지선 설정 완료";

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
        QString name = data["name"].toString();
        QString mode = data["mode"].toString();

        qDebug() << "[TCP] 객체 탐지선 응답 - index:" << index
                 << "name:" << name << "mode:" << mode << "성공:" << success;

        message = QString("객체 탐지선 '%1' (index: %2, mode: %3) 설정 %4")
                      .arg(name).arg(index).arg(mode).arg(success ? "성공" : "실패");
    }

    emit detectionLineConfirmed(success, message);

    if (success) {
        emit statusUpdated("객체 탐지선 설정 완료");
    } else {
        emit errorOccurred("객체 탐지선 설정 실패: " + message);
    }
}

// handleRoadLineResponse 함수 추가 (handleDetectionLineResponse 함수 다음에)
void TcpCommunicator::handleRoadLineResponse(const QJsonObject &jsonObj)
{
    // 서버에서 도로 기준선 설정에 대한 응답 처리
    bool success = true;  // 기본값
    QString message = "도로 기준선 설정 완료";

    if (jsonObj.contains("success")) {
        success = jsonObj["success"].toBool();
    }

    if (jsonObj.contains("message")) {
        message = jsonObj["message"].toString();
    }

    // data 필드에서 추가 정보 추출
    if (jsonObj.contains("data") && jsonObj["data"].isObject()) {
        QJsonObject data = jsonObj["data"].toObject();
        int matrixNum = data["matrixNum"].toInt();
        int x1 = data["x1"].toInt();
        int x2 = data["x2"].toInt();

        qDebug() << "[TCP] 도로 기준선 응답 - matrixNum:" << matrixNum
                 << "x1:" << x1 << "x2:" << x2 << "성공:" << success;

        message = QString("도로 기준선 (매트릭스: %1, x1:%2, x2:%3) 설정 %4")
                      .arg(matrixNum).arg(x1).arg(x2).arg(success ? "성공" : "실패");
    }

    emit roadLineConfirmed(success, message);

    if (success) {
        emit statusUpdated("도로 기준선 설정 완료");
    } else {
        emit errorOccurred("도로 기준선 설정 실패: " + message);
    }
}

void TcpCommunicator::handleCategorizedCoordinatesResponse(const QJsonObject &jsonObj)
{
    bool success = jsonObj["success"].toBool();
    QString message = jsonObj["message"].toString();

    qDebug() << "[TCP] 카테고리별 좌표 응답 - 성공:" << success << "메시지:" << message;

    if (jsonObj.contains("data") && jsonObj["data"].isObject()) {
        QJsonObject data = jsonObj["data"].toObject();
        int roadLinesProcessed = data["road_lines_processed"].toInt();
        int detectionLinesProcessed = data["detection_lines_processed"].toInt();
        int totalProcessed = data["total_processed"].toInt();

        qDebug() << "[TCP] 처리된 좌표 - 도로선:" << roadLinesProcessed << "개, 탐지선:" << detectionLinesProcessed << "개, 총:" << totalProcessed << "개";

        emit categorizedCoordinatesConfirmed(success, message, roadLinesProcessed, detectionLinesProcessed);
    } else {
        emit categorizedCoordinatesConfirmed(success, message, 0, 0);
    }

    if (success) {
        emit statusUpdated("카테고리별 좌표 전송 완료");
    } else {
        emit errorOccurred("카테고리별 좌표 전송 실패: " + message);
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
    int requestId = jsonObj["request_id"].toInt();

    qDebug() << QString("[TCP] JSON %1 - request_id: %2").arg(direction).arg(requestId);

// 디버그 모드에서만 전체 JSON 출력
#ifdef QT_DEBUG
    QJsonDocument doc(jsonObj);
    qDebug() << "JSON 내용:" << doc.toJson(QJsonDocument::Compact);
#endif
}
