#ifndef TCPCOMMUNICATOR_H
#define TCPCOMMUNICATOR_H

#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDateTime>

// 메시지 타입 열거형
enum class MessageType {
    REQUEST_IMAGES,
    IMAGES_RESPONSE,
    SEND_COORDINATES,
    COORDINATES_RESPONSE,
    STATUS_UPDATE,
    ERROR_RESPONSE
};

// 이미지 데이터 구조체
struct ImageData {
    QString imagePath;
    QString timestamp;
    QString logText;
    QString detectionType;
    QString direction;
};

class TcpCommunicator : public QObject
{
    Q_OBJECT

public:
    explicit TcpCommunicator(QObject *parent = nullptr);
    ~TcpCommunicator();

    // 연결 관리
    void connectToServer(const QString &host, int port);
    void disconnectFromServer();
    bool isConnectedToServer() const;

    // 메시지 전송
    bool sendLineCoordinates(int x1, int y1, int x2, int y2);
    void requestImageData(const QString &date = QString(), int hour = -1);

    // 설정
    void setConnectionTimeout(int timeoutMs);
    void setReconnectEnabled(bool enabled);

signals:
    void connected();
    void disconnected();
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
    bool sendJsonMessage(const QJsonObject &message);
    void processJsonMessage(const QJsonObject &jsonObj);
    void handleImagesResponse(const QJsonObject &jsonObj);
    void handleCoordinatesResponse(const QJsonObject &jsonObj);
    void handleStatusUpdate(const QJsonObject &jsonObj);
    void handleErrorResponse(const QJsonObject &jsonObj);

    // Base64 이미지 처리 함수 추가
    QString saveBase64Image(const QString &base64Data, const QString &timestamp);

    // 유틸리티 함수
    QJsonObject createBaseMessage(const QString &type) const;
    QString messageTypeToString(MessageType type) const;
    MessageType stringToMessageType(const QString &typeStr) const;
    void logJsonMessage(const QJsonObject &jsonObj, bool outgoing) const;

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
