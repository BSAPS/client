#include "TcpCommunicator.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QStandardPaths>
#include <QPixmap>
#include <QBuffer>
#include <QDateTime>
#include <QFile>
#include <QTimer>
#include <QImageReader>

TcpCommunicator::TcpCommunicator(QObject *parent)
    : QObject(parent)
    , m_socket(nullptr)
    , m_isConnected(false)
    , m_currentRequestId(0)
    , m_expectedDataSize(0)
    , m_receivedDataSize(0)
    , m_waitingForMoreData(false)
    , m_lastBufferSize(0)
{
    m_socket = new QTcpSocket(this);

    connect(m_socket, &QTcpSocket::connected, this, &TcpCommunicator::onSocketConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &TcpCommunicator::onSocketDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &TcpCommunicator::onDataReceived);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
            this, &TcpCommunicator::onSocketError);

    // 데이터 처리 지연 타이머
    m_processingTimer = new QTimer(this);
    m_processingTimer->setSingleShot(true);
    m_processingTimer->setInterval(2000); // 2초 대기
    connect(m_processingTimer, &QTimer::timeout, this, &TcpCommunicator::onProcessingTimeout);
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

    // 버퍼 초기화
    m_receivedBuffer.clear();
    m_imageBuffer.clear();
    m_waitingForMoreData = false;
    m_lastBufferSize = 0;

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

    qDebug() << "TCP 데이터 수신, 이번 크기:" << data.size() << "바이트, 총 버퍼 크기:" << m_receivedBuffer.size();

    // 데이터 수신 상태 체크
    if (m_receivedBuffer.size() == m_lastBufferSize) {
        // 더 이상 데이터가 오지 않으면 처리
        if (!m_waitingForMoreData) {
            m_waitingForMoreData = true;
            m_processingTimer->start();
        }
    } else {
        // 새로운 데이터가 왔으면 타이머 재시작
        m_lastBufferSize = m_receivedBuffer.size();
        m_waitingForMoreData = false;
        m_processingTimer->stop();
        m_processingTimer->start();
    }
}

void TcpCommunicator::onProcessingTimeout()
{
    qDebug() << "데이터 수신 완료, 처리 시작. 총 크기:" << m_receivedBuffer.size();

    if (m_receivedBuffer.size() > 0) {
        // 디버그용 원본 데이터 저장
        saveRawDataForDebug(m_receivedBuffer);

        // 바이너리 이미지 데이터 처리
        processBinaryImageData(m_receivedBuffer);
    }

    m_waitingForMoreData = false;
}

void TcpCommunicator::saveRawDataForDebug(const QByteArray &data)
{
    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/CCTVImages";
    QDir().mkpath(tempDir);

    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss-zzz");
    QString fileName = QString("raw_data_%1.bin").arg(timestamp);
    QString filePath = tempDir + "/" + fileName;

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(data);
        file.close();
        qDebug() << "원본 데이터 저장됨:" << filePath << "크기:" << data.size();
    }
}

void TcpCommunicator::processReceivedData(const QByteArray &data)
{
    qDebug() << "수신된 데이터:" << QString::fromUtf8(data);

    // 데이터가 JSON인지 확인 ('{' 또는 '[' 로 시작하는지 체크)
    QByteArray trimmedData = data.trimmed();
    if (trimmedData.startsWith('{') || trimmedData.startsWith('[')) {
        // JSON 데이터 처리
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(trimmedData, &error);

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
    } else {
        // 바이너리 이미지 데이터 처리
        processBinaryImageData(data);
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

void TcpCommunicator::processBinaryImageData(const QByteArray &data)
{
    qDebug() << "바이너리 이미지 데이터 처리 시작, 크기:" << data.size();

    QList<ImageData> imageList;
    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/CCTVImages";
    QDir().mkpath(tempDir);

    // 방법 1: JPEG 마커를 이용한 분리
    m_imageBuffer = data; // 전체 데이터를 이미지 버퍼에 저장
    tryExtractJpegImages(imageList, tempDir);

    // 방법 2: 다른 이미지 포맷 시도
    if (imageList.isEmpty()) {
        tryDifferentImageFormats(data, imageList, tempDir);
    }

    // 방법 3: 데이터를 여러 부분으로 나누어 시도
    if (imageList.isEmpty() && data.size() > 2000) {
        qDebug() << "데이터 분할 처리 시도";

        // 데이터를 여러 크기로 나누어 시도
        QList<int> chunkSizes = {data.size(), data.size()/2, data.size()/3, data.size()/4};

        for (int chunkSize : chunkSizes) {
            if (chunkSize < 500) continue;

            for (int i = 0; i < data.size(); i += chunkSize) {
                QByteArray chunk = data.mid(i, chunkSize);
                if (chunk.size() < 500) continue;

                QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss");
                QString fileName = QString("chunk_%1_%2_%3.jpg").arg(timestamp).arg(chunkSize).arg(i/chunkSize);
                QString filePath = tempDir + "/" + fileName;

                QFile file(filePath);
                if (file.open(QIODevice::WriteOnly)) {
                    file.write(chunk);
                    file.close();

                    // 다양한 이미지 리더로 시도
                    QImageReader reader(filePath);
                    if (reader.canRead()) {
                        QImage image = reader.read();
                        if (!image.isNull()) {
                            ImageData imgData;
                            imgData.imagePath = filePath;
                            imgData.timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
                            imgData.logText = QString("분할 이미지 %1 (크기: %2)").arg(i/chunkSize + 1).arg(chunk.size());

                            imageList.append(imgData);
                            qDebug() << "분할 이미지 성공:" << filePath;
                        } else {
                            QFile::remove(filePath);
                        }
                    } else {
                        QFile::remove(filePath);
                    }
                }
            }

            if (!imageList.isEmpty()) break;
        }
    }

    // 결과 전송
    if (!imageList.isEmpty()) {
        qDebug() << "총" << imageList.size() << "개 이미지 처리 완료";
    } else {
        qDebug() << "이미지 처리 실패 - 빈 결과 전송";
    }

    emit imagesReceived(imageList);
}

void TcpCommunicator::tryExtractJpegImages(QList<ImageData> &imageList, const QString &tempDir)
{
    // JPEG 시작 마커들 (다양한 형태)
    QList<QByteArray> jpegHeaders = {
        QByteArray::fromHex("FFD8FFE0"), // JFIF
        QByteArray::fromHex("FFD8FFE1"), // EXIF
        QByteArray::fromHex("FFD8FFDB"), // 표준 JPEG
        QByteArray::fromHex("FFD8")      // 기본 JPEG 시작
    };

    QByteArray jpegEnd = QByteArray::fromHex("FFD9");

    for (const QByteArray &header : jpegHeaders) {
        int startPos = 0;
        int imageCount = 0;

        while (true) {
            int jpegStart = m_imageBuffer.indexOf(header, startPos);
            if (jpegStart == -1) break;

            int jpegEndPos = m_imageBuffer.indexOf(jpegEnd, jpegStart);
            if (jpegEndPos == -1) {
                // 끝 마커가 없으면 다음 JPEG 시작까지 또는 버퍼 끝까지
                int nextJpegStart = m_imageBuffer.indexOf(header, jpegStart + header.size());
                if (nextJpegStart != -1) {
                    jpegEndPos = nextJpegStart - 2;
                } else {
                    jpegEndPos = m_imageBuffer.size() - 2;
                }
            }
            jpegEndPos += 2; // FFD9 마커 포함

            // 개별 이미지 데이터 추출
            QByteArray imageData = m_imageBuffer.mid(jpegStart, jpegEndPos - jpegStart);

            if (imageData.size() > 500) { // 최소 크기 체크
                QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss");
                QString fileName = QString("extracted_%1_%2.jpg").arg(timestamp).arg(imageCount);
                QString filePath = tempDir + "/" + fileName;

                QFile file(filePath);
                if (file.open(QIODevice::WriteOnly)) {
                    file.write(imageData);
                    file.close();

                    // 이미지 유효성 검사
                    QImageReader reader(filePath);
                    if (reader.canRead()) {
                        QImage image = reader.read();
                        if (!image.isNull()) {
                            ImageData imgData;
                            imgData.imagePath = filePath;
                            imgData.timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
                            imgData.logText = QString("추출된 이미지 %1: %2 (크기: %3)").arg(imageCount + 1).arg(imgData.timestamp).arg(imageData.size());

                            imageList.append(imgData);
                            qDebug() << "JPEG 이미지 추출 성공:" << filePath << "크기:" << image.size();
                            imageCount++;
                        } else {
                            QFile::remove(filePath);
                        }
                    } else {
                        QFile::remove(filePath);
                    }
                }
            }

            startPos = jpegEndPos;
        }

        if (imageCount > 0) {
            qDebug() << QString("헤더 %1로 %2개 이미지 추출 성공").arg(QString(header.toHex())).arg(imageCount);
            break;
        }
    }
}

void TcpCommunicator::tryDifferentImageFormats(const QByteArray &data, QList<ImageData> &imageList, const QString &tempDir)
{
    // 다양한 이미지 포맷으로 시도
    QStringList formats = {"jpg", "jpeg", "png", "bmp", "gif", "tiff"};

    for (const QString &format : formats) {
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss-zzz");
        QString fileName = QString("test_image_%1.%2").arg(timestamp).arg(format);
        QString filePath = tempDir + "/" + fileName;

        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(data);
            file.close();

            QImageReader reader(filePath);
            reader.setFormat(format.toUtf8());

            if (reader.canRead()) {
                QImage image = reader.read();
                if (!image.isNull()) {
                    ImageData imgData;
                    imgData.imagePath = filePath;
                    imgData.timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
                    imgData.logText = QString("%1 포맷 이미지: %2").arg(format.toUpper()).arg(imgData.timestamp);

                    imageList.append(imgData);
                    qDebug() << format << "포맷으로 이미지 로드 성공:" << filePath << "크기:" << image.size();
                    break; // 성공하면 다른 포맷은 시도하지 않음
                } else {
                    QFile::remove(filePath);
                }
            } else {
                QFile::remove(filePath);
            }
        }
    }
}

void TcpCommunicator::processImageBuffer()
{
    // 이 함수는 더 이상 사용하지 않음
}
