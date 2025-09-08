#ifndef VIDEOGRAPHICSVIEW_H
#define VIDEOGRAPHICSVIEW_H

#include "TcpCommunicator.h"

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsVideoItem>
#include <QGraphicsLineItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>

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

#endif // VIDEOGRAPHICSVIEW_H


