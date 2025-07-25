#include "bboxreceiver.h"
#include "BBox.h"

BBoxReceiver::BBoxReceiver(QSslSocket *socket, QObject *parent)
    : QObject(parent), m_socket(socket) {}

void BBoxReceiver::startReceiving() {
    m_running = true;
    connect(m_socket, &QSslSocket::readyRead, this, &BBoxReceiver::onReadyRead);
    qDebug() << "[BBoxReceiver] Started receiving.";
}

void BBoxReceiver::stopReceiving() {
    m_running = false;
    disconnect(m_socket, &QSslSocket::readyRead, this, &BBoxReceiver::onReadyRead);
    qDebug() << "[BBoxReceiver] Stopped receiving.";
}

void BBoxReceiver::onReadyRead() {
    if (!m_running) return;

    m_buffer.append(m_socket->readAll());
    qDebug() << "[BBoxReceiver] onReadyRead: Buffer size:" << m_buffer.size();

    while (m_buffer.size() >= 4) {
        quint32 len = qFromBigEndian<quint32>((uchar*)m_buffer.constData());
        qDebug() << "[BBoxReceiver] Trying to read message of length:" << len;

        // 비정상적으로 큰 메시지 크기 체크 (최대 10MB)
        if (len > 10 * 1024 * 1024) {
            qDebug() << "[BBoxReceiver] ❌ ERROR: Message too large:" << len << "bytes. Clearing buffer.";
            m_buffer.clear();
            break;
        }

        // 음수 또는 0 크기 체크
        if (len == 0) {
            qDebug() << "[BBoxReceiver] ❌ ERROR: Zero length message. Skipping 4 bytes.";
            m_buffer.remove(0, 4);
            continue;
        }

        if (m_buffer.size() < (int)(len + 4)) {
            qDebug() << "[BBoxReceiver] Incomplete message. Waiting for more data.";
            break;
        }

        QByteArray jsonBytes = m_buffer.mid(4, len);
        m_buffer.remove(0, len + 4);

        qDebug() << "[BBoxReceiver] Received JSON Data:" << QString::fromUtf8(jsonBytes);

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(jsonBytes, &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject()) {
            qDebug() << "[BBoxReceiver] JSON Parse Error:" << err.errorString();
            continue;
        }

        QJsonObject root = doc.object();
        if (root.contains("bboxes") && root["bboxes"].isArray() && 
            root.value("response_id").toInt() == 200) {
            QJsonArray bboxArray = root["bboxes"].toArray();
            QList<BBox> boxes;
            
            for (const QJsonValue &val : bboxArray) {
                if (!val.isObject()) continue;
                QJsonObject obj = val.toObject();

                BBox box;
                box.object_id = obj.value("id").toInt();
                box.rect = QRect(
                    obj.value("x").toInt(),
                    obj.value("y").toInt(),
                    obj.value("width").toInt(),
                    obj.value("height").toInt()
                );
                box.type = obj.value("type").toString();
                box.confidence = float(obj.value("confidence").toDouble());

                boxes.append(box);
            }
            
            qDebug() << "[BBoxReceiver] Successfully parsed" << boxes.size() << "bboxes, emitting signal.";
            
            // Get timestamp from JSON or use current time
            qint64 timestamp = QDateTime::currentMSecsSinceEpoch();
            if (root.contains("timestamp")) {
                timestamp = root["timestamp"].toVariant().toLongLong();
            }
            
            emit bboxesParsed(boxes, timestamp);
        } else {
            qDebug() << "[BBoxReceiver] Unexpected JSON format or response_id.";
        }
    }
}

QByteArray BBoxReceiver::readExactly(int n) {
    QByteArray buffer;
    int attempts = 0;
    const int maxAttempts = 10;

    while (buffer.size() < n && attempts++ < maxAttempts) {
        if (m_socket->bytesAvailable() > 0) {
            buffer.append(m_socket->read(n - buffer.size()));
        }
        if (buffer.size() < n) {
            if (!m_socket->waitForReadyRead(100)) {
                qDebug() << "[BBoxReceiver] Timeout waiting for data";
                break;
            }
        }
    }
    return buffer;
}



