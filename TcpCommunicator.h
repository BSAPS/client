#ifndef TCPCOMMUNICATOR_H
#define TCPCOMMUNICATOR_H

#include <QObject>
#include <QTimer>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDateTime>
#include <QThread>
#include <QRect>
#include <QSslSocket>
#include <QSslError>
#include <QSslConfiguration>

// Forward declarations
class VideoGraphicsView;

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

// 객체 탐지선 데이터 구조체
struct DetectionLineData {
    int index;              // 선 인덱스
    int x1, y1, x2, y2;     // 좌표
    QString name;           // 선 이름
    QString mode;           // "Right", "Left", "BothDirections"
    int leftMatrixNum;      // 왼쪽 매트릭스 번호
    int rightMatrixNum;     // 오른쪽 매트릭스 번호
};

// BBox 데이터 구조체
struct BBox {
    int object_id;          // 객체 ID
    QString type;           // 객체 타입 (예: "Vehicle", "Person" 등)
    double confidence;      // 신뢰도 (0.0 ~ 1.0)
    QRect rect;            // 바운딩 박스 영역 (x, y, width, height)
};

// 서버 양식에 맞춘 도로 기준선 데이터 구조체 수정
struct RoadLineData {
    int index;              // 기준선 번호
    int matrixNum1;         // 시작점 매트릭스 번호
    int x1, y1;             // 시작점 좌표
    int matrixNum2;         // 끝점 매트릭스 번호
    int x2, y2;             // 끝점 좌표
};

// 카테고리별 선 데이터 구조체 (기존 방식용)
// struct CategorizedLineData {
//     int x1;
//     int y1;
//     int x2;
//     int y2;
// };

class TcpCommunicator : public QObject
{
    Q_OBJECT

public:
    explicit TcpCommunicator(QObject *parent = nullptr);
    ~TcpCommunicator();

    // 연결 관리
    void connectToServer(const QString &host, quint16 port);
    void disconnectFromServer();
    bool isConnectedToServer() const;

    // 메시지 전송
    bool sendJsonMessage(const QJsonObject &message);

    // 데이터 전송 메서드들
    bool sendDetectionLine(const DetectionLineData &lineData);
    bool sendMultipleDetectionLines(const QList<DetectionLineData> &detectionLines);

    bool sendRoadLine(const RoadLineData &lineData);
    bool sendMultipleRoadLines(const QList<RoadLineData> &roadLines);
    void requestImageData(const QString &date = QString(), int hour = -1);

    // 저장된 선 데이터 요청
    bool requestSavedRoadLines();
    bool requestSavedDetectionLines();
    bool requestDeleteLines();

    // 설정
    void setConnectionTimeout(int timeoutMs);
    void setReconnectEnabled(bool enabled);
    void setVideoView(VideoGraphicsView* videoView);

signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString &error);
    void messageReceived(const QString &message);
    void imagesReceived(const QList<ImageData> &images);
    void coordinatesConfirmed(bool success, const QString &message);
    void detectionLineConfirmed(bool success, const QString &message);
    void statusUpdated(const QString &status);

    // signals 섹션에 시그널 추가
    void roadLineConfirmed(bool success, const QString &message);
    void savedRoadLinesReceived(const QList<RoadLineData> &roadLines);
    void savedDetectionLinesReceived(const QList<DetectionLineData> &detectionLines);

    void categorizedCoordinatesConfirmed(bool success, const QString &message, int roadLinesProcessed, int detectionLinesProcessed);

    // BBox 관련 시그널
    void bboxesReceived(const QList<BBox> &bboxes, qint64 timestamp);


private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onError(QAbstractSocket::SocketError error);

    void onConnectionTimeout();
    void onSslEncrypted();
    void onSslErrors(const QList<QSslError> &errors);

    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketReadyRead();
    void onSocketError(QAbstractSocket::SocketError error);
    void onReconnectTimer();

private:
    // JSON 메시지 처리
    void processJsonMessage(const QJsonObject &jsonObj);
    void handleImagesResponse(const QJsonObject &jsonObj);
    void handleStatusUpdate(const QJsonObject &jsonObj);
    void handleErrorResponse(const QJsonObject &jsonObj);

    // Base64 이미지 처리 함수 추가
    QString saveBase64Image(const QString &base64Data, const QString &timestamp);

    // 유틸리티 함수
    // QString messageTypeToString(MessageType type) const;
    // MessageType stringToMessageType(const QString &typeStr) const;
    void logJsonMessage(const QJsonObject &jsonObj, bool outgoing) const;
    void startReconnectTimer();
    void stopReconnectTimer();

    // 네트워크 관련
    QSslSocket *m_socket;
    QTimer *m_connectionTimer;
    QTimer *m_reconnectTimer;
    QString m_host;
    quint16 m_port;
    bool m_isConnected;
    QString m_receivedData;

    bool m_autoReconnect;

    VideoGraphicsView *m_videoView;

    // 설정
    int m_connectionTimeoutMs;
    bool m_reconnectEnabled;
    int m_reconnectAttempts;
    int m_maxReconnectAttempts;
    int m_reconnectDelayMs;

    // private 섹션에 함수 선언 추가
    void setupSslConfiguration();

    // 저장된 선 데이터 응답 처리 함수들
    void handleDetectionLinesFromServer(const QJsonObject &jsonObj);
    void handleRoadLinesFromServer(const QJsonObject &jsonObj);

    // BBox 처리 함수
    void handleBBoxResponse(const QJsonObject &jsonObj);

    // 저장된 선 데이터 관리
    QList<RoadLineData> m_receivedRoadLines;
    QList<DetectionLineData> m_receivedDetectionLines;
    bool m_roadLinesReceived;
    bool m_detectionLinesReceived;

    void checkAndEmitAllLinesReceived();
};

#endif // TCPCOMMUNICATOR_H
