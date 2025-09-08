#ifndef LINEDRAWINGDIALOG_H
#define LINEDRAWINGDIALOG_H

#include "TcpCommunicator.h"
#include "VideoGraphicsView.h"

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QVideoWidget>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QSlider>
#include <QTimer>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPoint>
#include <QList>
#include <QPair>
#include <QTextEdit>
#include <QTime>
#include <QRadioButton>
#include <QButtonGroup>
#include <QFrame>
#include <QInputDialog>

class CustomTitleBar;

/**
 * @brief 기준선/감지선 그리기 다이얼로그
 * @details 영상 위에 선을 그리고, TCP로 좌표를 전송하는 UI 및 로직을 담당합니다.
 */
class LineDrawingDialog : public QDialog
{
    Q_OBJECT

public:

    /**
     * @brief 생성자 (TCP 미사용)
     * @param rtspUrl RTSP URL
     * @param parent 부모 위젯
     */
    explicit LineDrawingDialog(const QString &rtspUrl, QWidget *parent = nullptr);

    /**
     * @brief 생성자 (TCP 사용)
     * @param rtspUrl RTSP URL
     * @param tcpCommunicator TCP 통신 객체
     * @param parent 부모 위젯
     */
    explicit LineDrawingDialog(const QString &rtspUrl, TcpCommunicator* tcpCommunicator, QWidget *parent = nullptr);

    /**
     * @brief 소멸자
     */
    ~LineDrawingDialog();

    // TCP 통신기 설정 메서드
    /**
     * @brief TCP 통신기 설정
     * @param communicator TCP 통신 객체
     */
    void setTcpCommunicator(TcpCommunicator* communicator);

signals:
    /**
     * @brief 카테고리별 선 데이터 시그널
     * @param roadLines 도로선 리스트
     * @param detectionLines 감지선 리스트
     */
    void categorizedLinesReady(const QList<RoadLineData> &roadLines, const QList<DetectionLineData> &detectionLines);

private slots:
    /** @brief 그리기 시작 버튼 클릭 슬롯 */
    void onStartDrawingClicked();
    /** @brief 그리기 종료 버튼 클릭 슬롯 */
    void onStopDrawingClicked();
    /** @brief 선 전체 삭제 슬롯 */
    void onClearLinesClicked();
    /** @brief 좌표 전송 버튼 클릭 슬롯 */
    void onSendCoordinatesClicked();
    /**
     * @brief 선 그리기 완료 시 슬롯
     * @param start 시작점
     * @param end 끝점
     * @param category 선 카테고리
     */
    void onLineDrawn(const QPoint &start, const QPoint &end, LineCategory category);
    /** @brief 로그 지우기 버튼 클릭 슬롯 */
    void onClearLogClicked();
    /**
     * @brief 플레이어 상태 변경 슬롯
     * @param state 상태
     */
    void onPlayerStateChanged(QMediaPlayer::PlaybackState state);
    /**
     * @brief 플레이어 에러 슬롯
     * @param error 에러 코드
     * @param errorString 에러 메시지
     */
    void onPlayerError(QMediaPlayer::Error error, const QString &errorString);
    /**
     * @brief 미디어 상태 변경 슬롯
     * @param status 상태
     */
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);
    /** @brief 카테고리 변경 슬롯 */
    void onCategoryChanged();
    /** @brief 카테고리 정보 업데이트 */
    void updateCategoryInfo();
    /**
     * @brief 좌표 클릭 슬롯
     * @param lineIndex 선 인덱스
     * @param coordinate 좌표
     * @param isStartPoint 시작점 여부
     */
    void onCoordinateClicked(int lineIndex, const QPoint &coordinate, bool isStartPoint);
    /** @brief 저장된 선 불러오기 슬롯 */
    void onLoadSavedLinesClicked();

    // 저장된 선 데이터 수신 슬롯들
    /**
     * @brief 저장된 도로선 수신 슬롯
     * @param roadLines 도로선 리스트
     */
    void onSavedRoadLinesReceived(const QList<RoadLineData> &roadLines);
    /**
     * @brief 저장된 감지선 수신 슬롯
     * @param detectionLines 감지선 리스트
     */
    void onSavedDetectionLinesReceived(const QList<DetectionLineData> &detectionLines);

    // BBox 관련 슬롯
    /**
     * @brief BBox 데이터 수신 슬롯
     * @param bboxes BBox 리스트
     * @param timestamp 타임스탬프
     */
    void onBBoxesReceived(const QList<BBox> &bboxes, qint64 timestamp);
    /** @brief BBox ON 버튼 클릭 슬롯 */
    void onBBoxOnClicked();
    /** @brief BBox OFF 버튼 클릭 슬롯 */
    void onBBoxOffClicked();

private:
    // 좌표별 Matrix 매핑 저장
    /** @brief 좌표별 Matrix 매핑 리스트 */
    QList<CoordinateMatrixMapping> m_coordinateMatrixMappings;

    // UI 컴포넌트
    /** @brief 매핑 개수 라벨 */
    QLabel *m_mappingCountLabel;
    /** @brief 매핑 전송 버튼 */
    QPushButton *m_sendMappingsButton;
    /** @brief 메인 레이아웃 */
    QVBoxLayout *m_mainLayout;
    /** @brief 버튼 레이아웃 */
    QHBoxLayout *m_buttonLayout;
    /** @brief 비디오 뷰 */
    VideoGraphicsView *m_videoView;
    /** @brief 그리기 시작 버튼 */
    QPushButton *m_startDrawingButton;
    /** @brief 그리기 종료 버튼 */
    QPushButton *m_stopDrawingButton;
    /** @brief 선 전체 삭제 버튼 */
    QPushButton *m_clearLinesButton;
    /** @brief 좌표 전송 버튼 */
    QPushButton *m_sendCoordinatesButton;
    /** @brief 닫기 버튼 */
    QPushButton *m_closeButton;

    // BBox 관련 UI
    /** @brief BBox ON 버튼 */
    QPushButton *m_bboxOnButton;
    /** @brief BBox OFF 버튼 */
    QPushButton *m_bboxOffButton;
    /** @brief BBox 활성화 여부 */
    bool m_bboxEnabled;

    // 로그 관련 UI
    /** @brief 로그 텍스트 에디트 */
    QTextEdit *m_logTextEdit;
    /** @brief 로그 개수 라벨 */
    QLabel *m_logCountLabel;
    /** @brief 로그 지우기 버튼 */
    QPushButton *m_clearLogButton;

    // 미디어 관련
    /** @brief 미디어 플레이어 */
    QMediaPlayer *m_mediaPlayer;
    /** @brief 오디오 출력 */
    QAudioOutput *m_audioOutput;

    // 상태 관리
    /** @brief RTSP URL */
    QString m_rtspUrl;
    /** @brief 그려진 선 리스트 */
    QList<QPair<QPoint, QPoint>> m_drawnLines;
    /** @brief 드로잉 모드 여부 */
    bool m_isDrawingMode;

    // 카테고리 선택 UI
    /** @brief 카테고리 위젯 */
    QWidget *m_categoryWidget;
    /** @brief 카테고리 레이아웃 */
    QHBoxLayout *m_categoryLayout;
    /** @brief 도로선 라디오 버튼 */
    QRadioButton *m_roadLineRadio;
    /** @brief 감지선 라디오 버튼 */
    QRadioButton *m_detectionLineRadio;
    /** @brief 카테고리 버튼 그룹 */
    QButtonGroup *m_categoryButtonGroup;
    /** @brief 카테고리 정보 라벨 */
    QLabel *m_categoryInfoLabel;

    // 카테고리별 통계 라벨
    /** @brief 도로선 개수 라벨 */
    QLabel *m_roadLineCountLabel;
    /** @brief 감지선 개수 라벨 */
    QLabel *m_detectionLineCountLabel;

    // 카테고리별 선 관리
    /** @brief 현재 카테고리 */
    LineCategory m_currentCategory;
    /** @brief 선택된 도로선 인덱스 */
    int m_selectedRoadLineIndex;
    /** @brief 도로선 선택 모드 여부 */
    bool m_roadLineSelectionMode;

    // TCP 통신 관련
    /** @brief TCP 통신 객체 */
    TcpCommunicator *m_tcpCommunicator;

    // 저장된 선 데이터 관리
    /** @brief 로드된 도로선 리스트 */
    QList<RoadLineData> m_loadedRoadLines;
    /** @brief 로드된 감지선 리스트 */
    QList<DetectionLineData> m_loadedDetectionLines;
    /** @brief 도로선 로드 여부 */
    bool m_roadLinesLoaded;
    /** @brief 감지선 로드 여부 */
    bool m_detectionLinesLoaded;

    /** @brief 커스텀 타이틀바 */
    CustomTitleBar *titleBar;

private:
    /**
     * @brief 매핑 정보 업데이트
     */
    void updateMappingInfo();
    /**
     * @brief 좌표 매핑 추가
     * @param lineIndex 선 인덱스
     * @param coordinate 좌표
     * @param isStartPoint 시작점 여부
     * @param matrixNum 매트릭스 번호
     */
    void addCoordinateMapping(int lineIndex, const QPoint &coordinate, bool isStartPoint, int matrixNum);
    /**
     * @brief 좌표 매핑 전체 삭제
     */
    void clearCoordinateMappings();
    /**
     * @brief 도로선 매핑 리스트 반환
     * @return 도로선 리스트
     */
    QList<RoadLineData> getCoordinateMappingsAsRoadLines() const;

    /**
     * @brief UI 설정
     */
    void setupUI();
    /**
     * @brief 미디어 플레이어 설정
     */
    void setupMediaPlayer();
    /**
     * @brief 비디오 스트림 시작
     */
    void startVideoStream();
    /**
     * @brief 비디오 스트림 정지
     */
    void stopVideoStream();
    /**
     * @brief 로그 메시지 추가
     * @param message 메시지
     * @param type 메시지 타입 (기본값: "INFO")
     */
    void addLogMessage(const QString &message, const QString &type = "INFO");
    /**
     * @brief 로그 전체 삭제
     */
    void clearLog();
    /**
     * @brief 버튼 상태 업데이트
     */
    void updateButtonStates();

    // 저장된 선 데이터 로드 관련 함수들
    /**
     * @brief TCP 연결 설정
     */
    void setupTcpConnection();
    /**
     * @brief 서버로부터 저장된 선 요청
     */
    void requestSavedLinesFromServer();
    /**
     * @brief 모든 선 데이터 로드 확인
     */
    void checkAndLoadAllLines();
};

#endif // LINEDRAWINGDIALOG_H
