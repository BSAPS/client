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

// Forward declarations
class VideoGraphicsView;

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

// 수직선 데이터 구조체
struct PerpendicularLineData {
    int index;              // 원본 감지선 번호
    double a;               // y = ax + b에서 a값 (기울기)
    double b;               // y = ax + b에서 b값 (y절편)
};

// 도로 기준선 데이터 구조체
struct RoadLineData {
    int index;              // 기준선 번호
    int matrixNum1;         // 시작점 매트릭스 번호
    int x1, y1;             // 시작점 좌표
    int matrixNum2;         // 끝점 매트릭스 번호
    int x2, y2;             // 끝점 좌표
};

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
    bool sendMessage(const QString &message);

    // 데이터 전송 메서드들
    bool sendLineCoordinates(int x1, int y1, int x2, int y2);
    bool sendDetectionLine(const DetectionLineData &lineData);
    bool sendMultipleDetectionLines(const QList<DetectionLineData> &detectionLines);
    bool sendRoadLine(const RoadLineData &lineData);
    bool sendMultipleRoadLines(const QList<RoadLineData> &roadLines);
    bool sendPerpendicularLine(const PerpendicularLineData &lineData);
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
    void roadLineConfirmed(bool success, const QString &message);
    void perpendicularLineConfirmed(bool success, const QString &message);
    void savedRoadLinesReceived(const QList<RoadLineData> &roadLines);
    void savedDetectionLinesReceived(const QList<DetectionLineData> &detectionLines);

private slots:
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketReadyRead();
    void onSocketError(QAbstractSocket::SocketError error);
    void onReconnectTimer();

private:
    // 네트워크 관련
    QTcpSocket *m_socket;
    QTimer *m_reconnectTimer;
    QString m_host;
    quint16 m_port;
    bool m_isConnected;
    QString m_receivedData;
    VideoGraphicsView *m_videoView;

    // 설정
    int m_connectionTimeoutMs;
    bool m_reconnectEnabled;
    int m_reconnectAttempts;
    int m_maxReconnectAttempts;
    int m_reconnectDelayMs;
    bool m_autoReconnect;

    // JSON 메시지 처리
    void processJsonMessage(const QJsonObject &jsonObj);
    void handleImagesResponse(const QJsonObject &jsonObj);
    void handleCoordinatesResponse(const QJsonObject &jsonObj);
    void handleDetectionLineResponse(const QJsonObject &jsonObj);
    void handleStatusUpdate(const QJsonObject &jsonObj);
    void handleErrorResponse(const QJsonObject &jsonObj);
    void handleRoadLineResponse(const QJsonObject &jsonObj);
    void handlePerpendicularLineResponse(const QJsonObject &jsonObj);
    void handleSavedRoadLinesResponse(const QJsonObject &jsonObj);
    void handleSavedDetectionLinesResponse(const QJsonObject &jsonObj);

    // Base64 이미지 처리
    QString saveBase64Image(const QString &base64Data, const QString &timestamp);

    // 유틸리티 함수
    QJsonObject createBaseMessage(const QString &type) const;
    void setupSocket();
    void startReconnectTimer();
    void stopReconnectTimer();
};

#endif // TCPCOMMUNICATOR_H
