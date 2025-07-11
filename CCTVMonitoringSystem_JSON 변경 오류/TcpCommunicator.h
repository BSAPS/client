#ifndef TCPCOMMUNICATOR_H
#define TCPCOMMUNICATOR_H

#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include <QList>
#include <QString>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDateTime>

struct ImageData {
    QString imagePath;
    QString timestamp;
    QString logText;
    QString detectionType;  // "person" 또는 "vehicle"
    QString direction;      // "incoming" 또는 "outgoing"

    ImageData() = default;
    ImageData(const QString &path, const QString &time, const QString &log,
              const QString &type = "", const QString &dir = "")
        : imagePath(path), timestamp(time), logText(log), detectionType(type), direction(dir) {}
};

// JSON 메시지 타입 정의
enum class MessageType {
    REQUEST_IMAGES,
    IMAGES_RESPONSE,
    SEND_COORDINATES,
    COORDINATES_RESPONSE,
    STATUS_UPDATE,
    ERROR_RESPONSE
};

class TcpCommunicator : public QObject
{
    Q_OBJECT

public:
    explicit TcpCommunicator(QObject *parent = nullptr);
    ~TcpCommunicator();

    // 기본 연결 관리
    void connectToServer(const QString &host, int port);
    void disconnectFromServer();
    bool isConnectedToServer() const;

    // JSON 기반 통신 메서드
    bool sendLineCoordinates(int x1, int y1, int x2, int y2);
    void requestImageData(const QString &date = "", int hour = -1);
    bool sendJsonMessage(const QJsonObject &message);

    // 설정 메서드
    void setConnectionTimeout(int timeoutMs);
    void setReconnectEnabled(bool enabled);

signals:
    void connectionStatusChanged(bool connected);
    void errorOccurred(const QString &error);
    void messageReceived(const QString &message);
    void imagesReceived(const QList<ImageData> &images);
    void coordinatesConfirmed(bool success, const QString &message);
    void statusUpdated(const QString &status);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onError(QAbstractSocket::SocketError error);
    void onConnectionTimeout();
    void attemptReconnection();

private:
    // JSON 메시지 처리
    void processJsonMessage(const QJsonObject &jsonObj);
    void handleImagesResponse(const QJsonObject &jsonObj);
    void handleCoordinatesResponse(const QJsonObject &jsonObj);
    void handleStatusUpdate(const QJsonObject &jsonObj);
    void handleErrorResponse(const QJsonObject &jsonObj);

    // 유틸리티 메서드
    QJsonObject createBaseMessage(const QString &type) const;
    QString messageTypeToString(MessageType type) const;
    MessageType stringToMessageType(const QString &typeStr) const;
    void logJsonMessage(const QJsonObject &jsonObj, bool outgoing = false) const;

    // 네트워크 관련
    QTcpSocket *m_socket;
    QTimer *m_connectionTimer;
    QTimer *m_reconnectTimer;
    QString m_host;
    int m_port;
    bool m_isConnected;
    QString m_receivedData;

    // 설정
    int m_connectionTimeoutMs;
    bool m_reconnectEnabled;
    int m_reconnectAttempts;
    int m_maxReconnectAttempts;
    int m_reconnectDelayMs;
};

#endif // TCPCOMMUNICATOR_H
