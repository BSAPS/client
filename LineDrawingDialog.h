#ifndef LINEDRAWINGDIALOG_H
#define LINEDRAWINGDIALOG_H

#include <QDialog>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsVideoItem>
#include <QGraphicsLineItem>
#include <QGraphicsEllipseItem>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QMutex>
#include <QMap>
#include <QRadioButton>
#include <QButtonGroup>
#include <QTextEdit>
#include <QDateTime>
#include "BBox.h"
#include "TcpCommunicator.h"

enum class LineCategory {
    ROAD_DEFINITION,    // 도로 명시선
    DETECTION_LINE,     // 객체 탐지선
    PERPENDICULAR_LINE  // 수직선
};

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
class VideoGraphicsView : public QGraphicsView {
    Q_OBJECT
public:
    explicit VideoGraphicsView(QWidget *parent = nullptr);
    void setBBoxes(const QList<BBox>& bboxes, qint64 timestamp);
    void setDrawingMode(bool enabled);
    void clearLines();
    QList<QPair<QPoint, QPoint>> getLines() const;
    QGraphicsVideoItem* getVideoItem() { return m_videoItem; }
    void setCurrentCategory(LineCategory category);
    QList<CategorizedLine> getCategorizedLines() const;
    void clearCategoryLines(LineCategory category);
    int getCategoryLineCount(LineCategory category) const;
    void clearHighlight();

signals:
    void lineDrawn(const QPoint &start, const QPoint &end, LineCategory category);
    void coordinateClicked(int lineIndex, const QPoint &coordinate, bool isStartPoint);

protected:
    void drawForeground(QPainter *painter, const QRectF &rect) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

public:
    QGraphicsPixmapItem *m_pixmapItem;

private:
    void redrawAllLines();
    QGraphicsLineItem* findClickedRoadLine(const QPointF &clickPos);
    void highlightRoadLine(int lineIndex);
    void highlightCoordinate(int lineIndex, bool isStartPoint);

    QGraphicsScene *m_scene;
    QGraphicsVideoItem *m_videoItem;
    bool m_drawingMode;
    bool m_drawing;
    QPointF m_startPoint;
    QPointF m_currentPoint;
    QList<QPair<QPoint, QPoint>> m_lines;
    QList<QGraphicsLineItem*> m_lineItems;
    QList<QGraphicsEllipseItem*> m_pointItems;
    QGraphicsLineItem *m_currentLineItem;
    LineCategory m_currentCategory;
    QList<CategorizedLine> m_categorizedLines;
    QMutex m_mutex;
    QMap<qint64, QList<BBox>> m_timestampedBBoxes;
    qint64 m_currentTimestamp;
    QList<BBox> m_bboxes;
    QList<QGraphicsRectItem*> m_bboxItems;  // BBox 그래픽 아이템들
    QList<QGraphicsTextItem*> m_bboxTextItems;  // BBox 텍스트 아이템들
};

class LineDrawingDialog : public QDialog {
    Q_OBJECT
public:
    explicit LineDrawingDialog(const QString &rtspUrl, QWidget *parent = nullptr);
    ~LineDrawingDialog();

signals:
    void lineCoordinatesReady(int x1, int y1, int x2, int y2);
    void categorizedLinesReady(const QList<RoadLineData> &roadLines, const QList<DetectionLineData> &detectionLines);
    void perpendicularLineGenerated(int detectionLineIndex, double a, double b);

public slots:
    void updateBBoxes(const QList<BBox>& bboxes, qint64 timestamp);
    void updateFrame(const QImage& frame);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

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
    void updateFrameCount();
    void onCategoryChanged();
    void onClearCategoryClicked();
    void updateCategoryInfo();
    void onCoordinateClicked(int lineIndex, const QPoint &coordinate, bool isStartPoint);

private:
    void setupUI();
    void setupMediaPlayer();
    void startVideoStream();
    void stopVideoStream();
    void addLogMessage(const QString &message, const QString &type = "INFO");
    void clearLog();
    void updateButtonStates();
    void updateMappingInfo();
    void addCoordinateMapping(int lineIndex, const QPoint &coordinate, bool isStartPoint, int matrixNum);
    void clearCoordinateMappings();
    PerpendicularLineData calculatePerpendicularLine(const QPoint &startPoint, const QPoint &endPoint, int index);
    void generatePerpendicularLine(const CategorizedLine &detectionLine, int index);
    QList<RoadLineData> getCoordinateMappingsAsRoadLines() const;

    // UI components
    VideoGraphicsView *m_videoView;
    QVBoxLayout *m_mainLayout;
    QHBoxLayout *m_buttonLayout;
    QPushButton *m_startDrawingButton;
    QPushButton *m_stopDrawingButton;
    QPushButton *m_clearLinesButton;
    QPushButton *m_sendCoordinatesButton;
    QPushButton *m_closeButton;
    QLabel *m_statusLabel;
    QLabel *m_frameCountLabel;
    QTextEdit *m_logTextEdit;
    QLabel *m_logCountLabel;
    QPushButton *m_clearLogButton;
    QWidget *m_categoryWidget;
    QHBoxLayout *m_categoryLayout;
    QRadioButton *m_roadLineRadio;
    QRadioButton *m_detectionLineRadio;
    QButtonGroup *m_categoryButtonGroup;
    QLabel *m_categoryInfoLabel;
    QLabel *m_roadLineCountLabel;
    QLabel *m_detectionLineCountLabel;
    QLabel *m_mappingCountLabel;
    QPushButton *m_clearMappingsButton;
    QPushButton *m_sendMappingsButton;
    QPushButton *m_toggleBboxButton;

    // Media components
    QString m_rtspUrl;
    QMediaPlayer *m_mediaPlayer;
    QAudioOutput *m_audioOutput;
    QTimer *m_frameTimer;

    // State management
    int m_frameCount;
    LineCategory m_currentCategory;
    int m_selectedRoadLineIndex;
    bool m_roadLineSelectionMode;
    bool m_isDrawingMode;
    bool m_bboxEnabled;
    qint64 m_currentTimestamp;
    qint64 m_lastFrameTimestamp;

    // Data management
    QList<CoordinateMatrixMapping> m_coordinateMatrixMappings;
    QList<BBox> m_currentBBoxes;
    QMutex m_bboxMutex;
    TcpCommunicator* m_tcpCommunicator;
    qint64 m_lastBBoxTimestamp;
    QMap<qint64, QList<BBox>> m_timestampedBBoxes;
};



#endif // LINEDRAWINGDIALOG_H
