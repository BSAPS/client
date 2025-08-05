#ifndef LINEDRAWINGDIALOG_H
#define LINEDRAWINGDIALOG_H

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
#include <QResizeEvent>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsVideoItem>
#include <QGraphicsLineItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QRadioButton>
#include <QButtonGroup>
#include <QFrame>
#include "TcpCommunicator.h"
#include <QInputDialog>

// 선 카테고리 열거형
enum class LineCategory {
    ROAD_DEFINITION,    // 도로 명시선
    OBJECT_DETECTION    // 객체 탐지선
};

// 카테고리별 선 정보 구조체
struct CategorizedLine {
    QPoint start;
    QPoint end;
    LineCategory category;
};

// 좌표별 Matrix 매핑 정보 구조체
struct CoordinateMatrixMapping {
    int lineIndex;          // 선 인덱스
    QPoint coordinate;      // 좌표
    bool isStartPoint;      // 시작점 여부
    int matrixNum;          // Matrix 번호 (1-4)
    QString displayName;    // 표시용 이름
};

// QGraphicsView 기반 비디오 뷰어
class VideoGraphicsView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit VideoGraphicsView(QWidget *parent = nullptr);
    void setDrawingMode(bool enabled);
    void clearLines();
    QList<QPair<QPoint, QPoint>> getLines() const;
    QGraphicsVideoItem* getVideoItem() { return m_videoItem; }
    void setCurrentCategory(LineCategory category);
    QList<CategorizedLine> getCategorizedLines() const;
    void clearCategoryLines(LineCategory category);
    int getCategoryLineCount(LineCategory category) const;
    void clearHighlight();

    // 저장된 선 데이터를 화면에 그리는 함수
    void loadSavedDetectionLines(const QList<DetectionLineData> &detectionLines);
    void loadSavedRoadLines(const QList<RoadLineData> &roadLines);
    QGraphicsScene* scene() { return m_scene; }

    // 즉시 테스트 선 그리기 함수
    void drawImmediateTestLines();

    // BBox 관련 함수
    void setBBoxes(const QList<BBox> &bboxes, qint64 timestamp);
    void clearBBoxes();
    void setOriginalVideoSize(const QSize &size) { m_originalVideoSize = size; }

signals:
    void lineDrawn(const QPoint &start, const QPoint &end, LineCategory category);
    void coordinateClicked(int lineIndex, const QPoint &coordinate, bool isStartPoint);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void redrawAllLines();
    QGraphicsLineItem* findClickedRoadLine(const QPointF &clickPos);
    void highlightRoadLine(int lineIndex);
    void highlightCoordinate(int lineIndex, bool isStartPoint);

    QGraphicsScene *m_scene;
    QGraphicsVideoItem *m_videoItem;
    bool m_drawingMode;
    bool m_drawing;
    QPoint m_startPoint;
    QPoint m_currentPoint;
    QList<QPair<QPoint, QPoint>> m_lines;
    QList<QGraphicsLineItem*> m_lineItems;
    QList<QGraphicsEllipseItem*> m_pointItems;
    QGraphicsLineItem *m_currentLineItem;
    LineCategory m_currentCategory;
    QList<CategorizedLine> m_categorizedLines;

    // BBox 관련 멤버 변수
    QList<QGraphicsRectItem*> m_bboxRectItems;     // BBox 사각형 아이템들
    QList<QGraphicsTextItem*> m_bboxTextItems;     // BBox 텍스트 아이템들
    QSize m_originalVideoSize;                      // 원본 비디오 크기
    QSize m_currentViewSize;                        // 현재 뷰 크기
};

class LineDrawingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LineDrawingDialog(const QString &rtspUrl, QWidget *parent = nullptr);
    explicit LineDrawingDialog(const QString &rtspUrl, TcpCommunicator* tcpCommunicator, QWidget *parent = nullptr);
    ~LineDrawingDialog();

    // TCP 통신기 설정 메서드
    void setTcpCommunicator(TcpCommunicator* communicator);

signals:
    void lineCoordinatesReady(int x1, int y1, int x2, int y2);
    void categorizedLinesReady(const QList<RoadLineData> &roadLines, const QList<DetectionLineData> &detectionLines);
    void perpendicularLineGenerated(int detectionLineIndex, double a, double b);

private slots:
    void onStartDrawingClicked();
    void onStopDrawingClicked();
    void onClearLinesClicked();
    void onSendCoordinatesClicked();
    void onLineDrawn(const QPoint &start, const QPoint &end, LineCategory category);
    void onClearLogClicked();
    void onPlayerStateChanged(QMediaPlayer::PlaybackState state);
    void onPlayerError(QMediaPlayer::Error error, const QString &errorString);
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void onCategoryChanged();
    void onClearCategoryClicked();
    void updateCategoryInfo();
    void onCoordinateClicked(int lineIndex, const QPoint &coordinate, bool isStartPoint);
    void onLoadSavedLinesClicked();

    // 저장된 선 데이터 수신 슬롯들
    void onSavedRoadLinesReceived(const QList<RoadLineData> &roadLines);
    void onSavedDetectionLinesReceived(const QList<DetectionLineData> &detectionLines);

    // BBox 관련 슬롯
    void onBBoxesReceived(const QList<BBox> &bboxes, qint64 timestamp);
    void onBBoxOnClicked();
    void onBBoxOffClicked();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    // 좌표별 Matrix 매핑 저장
    QList<CoordinateMatrixMapping> m_coordinateMatrixMappings;

    // UI 컴포넌트
    QLabel *m_mappingCountLabel;
    QPushButton *m_sendMappingsButton;
    QVBoxLayout *m_mainLayout;
    QHBoxLayout *m_buttonLayout;
    VideoGraphicsView *m_videoView;
    QPushButton *m_startDrawingButton;
    QPushButton *m_stopDrawingButton;
    QPushButton *m_clearLinesButton;
    QPushButton *m_sendCoordinatesButton;
    QPushButton *m_closeButton;

    // BBox 관련 UI
    QPushButton *m_bboxOnButton;
    QPushButton *m_bboxOffButton;
    bool m_bboxEnabled;

    // 로그 관련 UI
    QTextEdit *m_logTextEdit;
    QLabel *m_logCountLabel;
    QPushButton *m_clearLogButton;

    // 미디어 관련
    QMediaPlayer *m_mediaPlayer;
    QAudioOutput *m_audioOutput;

    // 상태 관리
    QString m_rtspUrl;
    QList<QPair<QPoint, QPoint>> m_drawnLines;
    bool m_isDrawingMode;
    // QTimer *m_frameTimer;
    // int m_frameCount;

    // 카테고리 선택 UI
    QWidget *m_categoryWidget;
    QHBoxLayout *m_categoryLayout;
    QRadioButton *m_roadLineRadio;
    QRadioButton *m_detectionLineRadio;
    QButtonGroup *m_categoryButtonGroup;
    QLabel *m_categoryInfoLabel;

    // 카테고리별 통계 라벨
    QLabel *m_roadLineCountLabel;
    QLabel *m_detectionLineCountLabel;

    // 카테고리별 선 관리
    LineCategory m_currentCategory;
    int m_selectedRoadLineIndex;
    bool m_roadLineSelectionMode;

    // TCP 통신 관련
    TcpCommunicator *m_tcpCommunicator;

    // 저장된 선 데이터 관리
    QList<RoadLineData> m_loadedRoadLines;
    QList<DetectionLineData> m_loadedDetectionLines;
    bool m_roadLinesLoaded;
    bool m_detectionLinesLoaded;

private:
    void updateMappingInfo();
    void addCoordinateMapping(int lineIndex, const QPoint &coordinate, bool isStartPoint, int matrixNum);
    void clearCoordinateMappings();
    PerpendicularLineData calculatePerpendicularLine(const QPoint &start, const QPoint &end, int detectionLineIndex);
    void generatePerpendicularLine(const CategorizedLine &detectionLine, int index);
    QList<RoadLineData> getCoordinateMappingsAsRoadLines() const;

    void setupUI();
    void setupMediaPlayer();
    void startVideoStream();
    void stopVideoStream();
    void addLogMessage(const QString &message, const QString &type = "INFO");
    void clearLog();
    void updateButtonStates();

    // 저장된 선 데이터 로드 관련 함수들
    void setupTcpConnection();
    void requestSavedLinesFromServer();
    void checkAndLoadAllLines();
};

#endif // LINEDRAWINGDIALOG_H
