#ifndef TCPCOMMUNICATOR_H
#define TCPCOMMUNICATOR_H

#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDateTime>
#include <QThread>
#include <QSslSocket>
#include <QSslError>





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

// 카테고리별 선 데이터 구조체 (기존 방식용)
struct CategorizedLineData {
    int x1;
    int y1;
    int x2;
    int y2;
};

// 서버 양식에 맞춘 객체 탐지선 데이터 구조체
struct DetectionLineData {
    int index;              // 선 인덱스
    int x1, y1, x2, y2;     // 좌표
    QString name;           // 선 이름
    QString mode;           // "Right", "Left", "BothDirections"
    int leftMatrixNum;      // 왼쪽 매트릭스 번호
    int rightMatrixNum;     // 오른쪽 매트릭스 번호
};

struct PerpendicularLineData {
    int index;              // 원본 감지선 번호
    double a;               // y = ax + b에서 a값 (기울기)
    double b;               // y = ax + b에서 b값 (y절편)
};

// 서버 양식에 맞춘 도로 기준선 데이터 구조체 추가
struct RoadLineData {
    int matrixNum;          // 매트릭스 번호 (1-4)
    int x1, y1, x2, y2;     // 시작점(x1,y1)과 끝점(x2,y2) 좌표
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
    bool sendDetectionLine(const DetectionLineData &lineData);
    bool sendMultipleDetectionLines(const QList<DetectionLineData> &detectionLines);
    bool sendCategorizedLineCoordinates(const QList<CategorizedLineData> &roadLines, const QList<CategorizedLineData> &detectionLines);
    void requestImageData(const QString &date = QString(), int hour = -1);

    // 설정
    void setConnectionTimeout(int timeoutMs);
    void setReconnectEnabled(bool enabled);

    // TcpCommunicator 클래스의 public 섹션에 함수 선언 추가
    bool sendRoadLine(const RoadLineData &lineData);
    bool sendMultipleRoadLines(const QList<RoadLineData> &roadLines);
    bool sendPerpendicularLine(const PerpendicularLineData &lineData);



signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString &error);
    void messageReceived(const QString &message);
    void imagesReceived(const QList<ImageData> &images);
    void coordinatesConfirmed(bool success, const QString &message);
    void detectionLineConfirmed(bool success, const QString &message);
    void categorizedCoordinatesConfirmed(bool success, const QString &message, int roadLinesProcessed, int detectionLinesProcessed);
    void statusUpdated(const QString &status);

    // signals 섹션에 시그널 추가
    void roadLineConfirmed(bool success, const QString &message);
    void perpendicularLineConfirmed(bool success, const QString &message);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onError(QAbstractSocket::SocketError error);
    void onConnectionTimeout();
    void attemptReconnection();
    void onSslEncrypted();
    void onSslErrors(const QList<QSslError> &errors);

private:
    // JSON 메시지 처리
    bool sendJsonMessage(const QJsonObject &message);
    void processJsonMessage(const QJsonObject &jsonObj);
    void handleImagesResponse(const QJsonObject &jsonObj);
    void handleCoordinatesResponse(const QJsonObject &jsonObj);
    void handleDetectionLineResponse(const QJsonObject &jsonObj);
    void handleCategorizedCoordinatesResponse(const QJsonObject &jsonObj);
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
    QSslSocket* m_socket;
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

    // private 섹션에 함수 선언 추가
    void handleRoadLineResponse(const QJsonObject &jsonObj);
    void handlePerpendicularLineResponse(const QJsonObject &jsonObj);
    void setupSslConfiguration();
};

#endif // TCPCOMMUNICATOR_H
