#include "TcpCommunicator.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QStandardPaths>
#include <QPixmap>
#include <QBuffer>

TcpCommunicator::TcpCommunicator(QObject *parent)
    : QObject(parent)
    , m_socket(nullptr)
    , m_isConnected(false)
    , m_currentRequestId(0)
    , m_expectedDataSize(0)
    , m_receivedDataSize(0)
{
    m_socket = new QTcpSocket(this);
    
    connect(m_socket, &QTcpSocket::connected, this, &TcpCommunicator::onSocketConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &TcpCommunicator::onSocketDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &TcpCommunicator::onDataReceived);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
            this, &TcpCommunicator::onSocketError);
}

TcpCommunicator::~TcpCommunicator()
{
    if (m_socket && m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->disconnectFromHost();
    }
}

void TcpCommunicator::connectToServer(const QString &host, int port)
{
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->disconnectFromHost();
    }
    
    qDebug() << QString("TCP 서버 연결 시도: %1:%2").arg(host).arg(port);
    m_socket->connectToHost(host, port);
}

void TcpCommunicator::disconnectFromServer()
{
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->disconnectFromHost();
    }
}

bool TcpCommunicator::isConnected() const
{
    return m_isConnected;
}

void TcpCommunicator::sendLineCoordinates(const QPoint &start, const QPoint &end)
{
    if (!m_isConnected) {
        qDebug() << "TCP 연결이 없어 좌표 전송 불가";
        return;
    }
    
    QJsonObject json;
    json["type"] = "line_coordinates";
    json["request_id"] = ++m_currentRequestId;
    json["start_x"] = start.x();
    json["start_y"] = start.y();
    json["end_x"] = end.x();
    json["end_y"] = end.y();

    QJsonDocument doc(json);
    QByteArray data = doc.toJson(QJsonDocument::Compact) + "\n";

    m_socket->write(data);
    qDebug() << "기준선 좌표 전송:" << data;
}

void TcpCommunicator::requestImagesByDateAndHour(const QString &date, int hour)
{
    if (!m_isConnected) {
        qDebug() << "TCP 연결이 없어 이미지 요청 불가";
        return;
    }
    
    QDate inputDate = QDate::fromString(date, "yyyy-MM-dd");
    QDateTime startDateTime = QDateTime(inputDate, QTime(hour, 0, 0));
    QDateTime endDateTime = startDateTime.addSecs(3600); // 1시간 후

    QString startTimeStr = startDateTime.toString("yyyy-MM-ddThh");
    QString endTimeStr = endDateTime.toString("yyyy-MM-ddThh");
    QString dateRangeStr = QString("1/1/%1/%2").arg(startTimeStr).arg(endTimeStr);

    QString message = dateRangeStr + "\n";
    QByteArray data = message.toUtf8();

    qint64 bytesWritten = m_socket->write(data);
    if (bytesWritten == -1) {
        qDebug() << "이미지 요청 실패:" << m_socket->errorString();
    } else {
        qDebug() << QString("이미지 요청 전송 (문자열): %1").arg(dateRangeStr);
    }
}

void TcpCommunicator::onSocketConnected()
{
    m_isConnected = true;
    qDebug() << "TCP 소켓 연결됨";
    emit connected();
}

void TcpCommunicator::onSocketDisconnected()
{
    m_isConnected = false;
    qDebug() << "TCP 소켓 연결 해제됨";
    emit disconnected();
}

void TcpCommunicator::onSocketError(QAbstractSocket::SocketError error)
{
    QString errorString;
    switch (error) {
    case QAbstractSocket::ConnectionRefusedError:
        errorString = "연결이 거부되었습니다.";
        break;
    case QAbstractSocket::RemoteHostClosedError:
        errorString = "원격 호스트가 연결을 종료했습니다.";
        break;
    case QAbstractSocket::HostNotFoundError:
        errorString = "호스트를 찾을 수 없습니다.";
        break;
    case QAbstractSocket::SocketTimeoutError:
        errorString = "연결 시간이 초과되었습니다.";
        break;
    case QAbstractSocket::NetworkError:
        errorString = "네트워크 오류가 발생했습니다.";
        break;
    default:
        errorString = QString("알 수 없는 오류: %1").arg(m_socket->errorString());
        break;
    }
    
    qDebug() << "TCP 소켓 에러:" << errorString;
    emit errorOccurred(errorString);
}

void TcpCommunicator::onDataReceived()
{
    QByteArray data = m_socket->readAll();
    m_receivedBuffer.append(data);
    
    // 줄 단위로 데이터 처리
    while (m_receivedBuffer.contains('\n')) {
        int newlineIndex = m_receivedBuffer.indexOf('\n');
        QByteArray line = m_receivedBuffer.left(newlineIndex);
        m_receivedBuffer.remove(0, newlineIndex + 1);
        
        processReceivedData(line);
    }
}

void TcpCommunicator::processReceivedData(const QByteArray &data)
{
    qDebug() << "수신된 데이터:" << data;
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        qDebug() << "JSON 파싱 에러:" << error.errorString();
        emit dataReceived(QString::fromUtf8(data));
        return;
    }
    
    QJsonObject json = doc.object();
    QString type = json["type"].toString();
    int requestId = json["request_id"].toInt();
    
    if (type == "response") {
        int success = json["success"].toInt();
        QString data1 = json["data1"].toString();
        QString data2 = json["data2"].toString();
        QString data3 = json["data3"].toString();
        
        emit packetReceived(requestId, success, data1, data2, data3);
        
    } else if (type == "images_response") {
        processImagesResponse(json);
        
    } else {
        emit dataReceived(QString::fromUtf8(data));
    }
}

void TcpCommunicator::processImagesResponse(const QJsonObject &json)
{
    QJsonArray imagesArray = json["images"].toArray();
    QList<ImageData> imageList;
    
    // 임시 이미지 저장 디렉토리 생성
    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/CCTVImages";
    QDir().mkpath(tempDir);
    
    for (const QJsonValue &value : imagesArray) {
        QJsonObject imageObj = value.toObject();
        
        ImageData imageData;
        imageData.timestamp = imageObj["timestamp"].toString();
        imageData.logText = imageObj["log_text"].toString();
        
        // Base64 이미지 데이터 디코딩
        QString base64Data = imageObj["image_data"].toString();
        QByteArray imageBytes = QByteArray::fromBase64(base64Data.toUtf8());
        
        // 임시 파일로 저장
        QString fileName = QString("image_%1.jpg").arg(imageData.timestamp).replace(":", "-").replace(" ", "_");
        QString filePath = tempDir + "/" + fileName;
        
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(imageBytes);
            file.close();
            imageData.imagePath = filePath;
            imageList.append(imageData);
            qDebug() << "이미지 저장됨:" << filePath;
        } else {
            qDebug() << "이미지 저장 실패:" << filePath;
        }
    }
    
    emit imagesReceived(imageList);
}
