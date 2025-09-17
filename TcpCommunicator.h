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

/**
 * @brief 메시지 타입 열거형
 * @details 서버와 클라이언트 간 통신에 사용되는 메시지 타입 정의
 */
enum class MessageType {
    REQUEST_IMAGES,
    IMAGES_RESPONSE,
    SEND_COORDINATES,
    COORDINATES_RESPONSE,
    STATUS_UPDATE,
    ERROR_RESPONSE
};

/**
 * @brief 이미지 데이터 구조체
 * @details 이미지 경로, 타임스탬프, 로그, 탐지 타입, 방향 정보 포함
 */
struct ImageData {
    QString imagePath;
    QString timestamp;
    QString logText;
    QString detectionType;
    QString direction;
};

/**
 * @brief 객체 탐지선 데이터 구조체
 * @details 선 인덱스, 좌표, 이름, 모드, 매트릭스 번호 포함
 */
struct DetectionLineData {
    int index;
    int x1, y1, x2, y2;
    QString name;
    QString mode;
    int leftMatrixNum;
    int rightMatrixNum;
};

/**
 * @brief BBox 데이터 구조체
 * @details 객체 ID, 타입, 신뢰도, 바운딩 박스 영역 포함
 */
struct BBox {
    int object_id;          // 객체 ID
    QString type;           // 객체 타입 (예: "Vehicle", "Person" 등)
    double confidence;      // 신뢰도 (0.0 ~ 1.0)
    QRect rect;            // 바운딩 박스 영역 (x, y, width, height)
};

/**
 * @brief 도로 기준선 데이터 구조체 (서버 양식)
 * @details 기준선 번호, 매트릭스 번호, 좌표 포함
 */
struct RoadLineData {
    int index;
    int matrixNum1;
    int x1, y1;
    int matrixNum2;
    int x2, y2;
};

/**
 * @brief TCP 통신 및 데이터 관리 클래스
 * @details 서버와의 연결, 메시지 송수신, 선/이미지 데이터 관리 등 담당
 */
class TcpCommunicator : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief TcpCommunicator 생성자
     * @param parent 부모 객체
     */
    explicit TcpCommunicator(QObject *parent = nullptr);
    /**
     * @brief 소멸자
     */
    ~TcpCommunicator();

    /**
     * @brief 서버에 연결
     * @param host 호스트 주소
     * @param port 포트 번호
     */
    void connectToServer(const QString &host, quint16 port);
    /**
     * @brief 서버 연결 해제
     */
    void disconnectFromServer();
    /**
     * @brief 서버 연결 여부 반환
     * @return 연결 여부
     */
    bool isConnectedToServer() const;

    /**
     * @brief JSON 메시지 전송
     * @param message 전송할 JSON 객체
     * @return 성공 여부
     */
    bool sendJsonMessage(const QJsonObject &message);

    /**
     * @brief 탐지선 데이터 전송
     * @param lineData 탐지선 데이터
     * @return 성공 여부
     */
    bool sendDetectionLine(const DetectionLineData &lineData);
    /**
     * @brief 여러 탐지선 데이터 전송
     * @param detectionLines 탐지선 데이터 리스트
     * @return 성공 여부
     */
    bool sendMultipleDetectionLines(const QList<DetectionLineData> &detectionLines);

    /**
     * @brief 도로선 데이터 전송
     * @param lineData 도로선 데이터
     * @return 성공 여부
     */
    bool sendRoadLine(const RoadLineData &lineData);
    /**
     * @brief 여러 도로선 데이터 전송
     * @param roadLines 도로선 데이터 리스트
     * @return 성공 여부
     */
    bool sendMultipleRoadLines(const QList<RoadLineData> &roadLines);
    /**
     * @brief 이미지 데이터 요청
     * @param date 날짜(선택)
     * @param hour 시간(선택)
     */
    void requestImageData(const QString &date = QString(), int hour = -1);

    /**
     * @brief 저장된 도로선 데이터 요청
     * @return 성공 여부
     */
    bool requestSavedRoadLines();
    /**
     * @brief 저장된 감지선 데이터 요청
     * @return 성공 여부
     */
    bool requestSavedDetectionLines();
    /**
     * @brief 저장된 선 데이터 삭제 요청
     * @return 성공 여부
     */
    bool requestDeleteLines();

    /**
     * @brief 연결 타임아웃 설정
     * @param timeoutMs 타임아웃(ms)
     */
    void setConnectionTimeout(int timeoutMs);
    /**
     * @brief 자동 재연결 활성화 설정
     * @param enabled 활성화 여부
     */
    void setReconnectEnabled(bool enabled);
    /**
     * @brief 비디오 뷰 설정
     * @param videoView VideoGraphicsView 포인터
     */
    void setVideoView(VideoGraphicsView* videoView);

signals:
    /** @brief 서버 연결됨 */
    void connected();
    /** @brief 서버 연결 해제됨 */
    void disconnected();
    /** @brief 에러 발생 */
    void errorOccurred(const QString &error);
    /** @brief 메시지 수신 */
    void messageReceived(const QString &message);
    /** @brief 이미지 데이터 수신 */
    void imagesReceived(const QList<ImageData> &images);
    /** @brief 좌표 전송 확인 */
    void coordinatesConfirmed(bool success, const QString &message);
    /** @brief 탐지선 전송 확인 */
    void detectionLineConfirmed(bool success, const QString &message);
    /** @brief 상태 업데이트 */
    void statusUpdated(const QString &status);
    /** @brief 도로선 전송 확인 */
    void roadLineConfirmed(bool success, const QString &message);
    /** @brief 저장된 도로선 수신 */
    void savedRoadLinesReceived(const QList<RoadLineData> &roadLines);
    /** @brief 저장된 감지선 수신 */
    void savedDetectionLinesReceived(const QList<DetectionLineData> &detectionLines);
    /** @brief 카테고리별 좌표 전송 확인 */
    void categorizedCoordinatesConfirmed(bool success, const QString &message, int roadLinesProcessed, int detectionLinesProcessed);
    /** @brief BBox 데이터 수신 */
    void bboxesReceived(const QList<BBox> &bboxes, qint64 timestamp);

private slots:
    /** @brief 서버 연결 슬롯 */
    void onConnected();
    /** @brief 서버 연결 해제 슬롯 */
    void onDisconnected();
    /** @brief 데이터 수신 슬롯 */
    void onReadyRead();
    /** @brief 에러 슬롯 */
    void onError(QAbstractSocket::SocketError error);
    /** @brief 연결 타임아웃 슬롯 */
    void onConnectionTimeout();
    /** @brief SSL 암호화 슬롯 */
    void onSslEncrypted();
    /** @brief SSL 에러 슬롯 */
    void onSslErrors(const QList<QSslError> &errors);
    /** @brief 소켓 연결 슬롯 */
    void onSocketConnected();
    /** @brief 소켓 연결 해제 슬롯 */
    void onSocketDisconnected();
    /** @brief 소켓 데이터 수신 슬롯 */
    void onSocketReadyRead();
    /** @brief 소켓 에러 슬롯 */
    void onSocketError(QAbstractSocket::SocketError error);
    /** @brief 재연결 타이머 슬롯 */
    void onReconnectTimer();

private:
    /** @brief JSON 메시지 처리 */
    void processJsonMessage(const QJsonObject &jsonObj);
    /** @brief 이미지 응답 처리 */
    void handleImagesResponse(const QJsonObject &jsonObj);
    /** @brief 상태 업데이트 처리 */
    void handleStatusUpdate(const QJsonObject &jsonObj);
    /** @brief 에러 응답 처리 */
    void handleErrorResponse(const QJsonObject &jsonObj);
    /** @brief Base64 이미지 저장 */
    QString saveBase64Image(const QString &base64Data, const QString &timestamp);
    /** @brief JSON 메시지 로깅 */
    void logJsonMessage(const QJsonObject &jsonObj, bool outgoing) const;
    /** @brief 재연결 타이머 시작 */
    void startReconnectTimer();
    /** @brief 재연결 타이머 중지 */
    void stopReconnectTimer();
    /** @brief SSL 설정 구성 */
    void setupSslConfiguration();
    /** @brief 감지선 데이터 응답 처리 */
    void handleDetectionLinesFromServer(const QJsonObject &jsonObj);
    /** @brief 도로선 데이터 응답 처리 */
    void handleRoadLinesFromServer(const QJsonObject &jsonObj);
    /** @brief BBox 응답 처리 */
    void handleBBoxResponse(const QJsonObject &jsonObj);
    /** @brief 모든 선 데이터 수신 완료 체크 및 시그널 발신 */
    void checkAndEmitAllLinesReceived();

    /** @brief 네트워크 소켓 */
    QSslSocket *m_socket;
    /** @brief 연결 타이머 */
    QTimer *m_connectionTimer;
    /** @brief 재연결 타이머 */
    QTimer *m_reconnectTimer;
    /** @brief 호스트 주소 */
    QString m_host;
    /** @brief 포트 번호 */
    quint16 m_port;
    /** @brief 연결 여부 */
    bool m_isConnected;
    /** @brief 수신 데이터 버퍼 */
    QString m_receivedData;
    /** @brief 자동 재연결 여부 */
    bool m_autoReconnect;
    /** @brief 비디오 뷰 포인터 */
    VideoGraphicsView *m_videoView;
    /** @brief 연결 타임아웃(ms) */
    int m_connectionTimeoutMs;
    /** @brief 재연결 활성화 여부 */
    bool m_reconnectEnabled;
    /** @brief 재연결 시도 횟수 */
    int m_reconnectAttempts;
    /** @brief 최대 재연결 시도 횟수 */
    int m_maxReconnectAttempts;
    /** @brief 재연결 지연(ms) */
    int m_reconnectDelayMs;
    /** @brief 저장된 도로선 리스트 */
    QList<RoadLineData> m_receivedRoadLines;
    /** @brief 저장된 감지선 리스트 */
    QList<DetectionLineData> m_receivedDetectionLines;
    /** @brief 도로선 수신 여부 */
    bool m_roadLinesReceived;
    /** @brief 감지선 수신 여부 */
    bool m_detectionLinesReceived;
};

#endif // TCPCOMMUNICATOR_H
