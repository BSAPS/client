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

/**
 * @brief 선 카테고리 열거형
 * @details 도로 명시선, 객체 탐지선 구분
 */
enum class LineCategory {
    ROAD_DEFINITION,    // 도로 명시선
    OBJECT_DETECTION    // 객체 탐지선
};

/**
 * @brief 카테고리별 선 정보 구조체
 * @details 시작점, 끝점, 카테고리 정보를 포함
 */
struct CategorizedLine {
    QPoint start;
    QPoint end;
    LineCategory category;
};

/**
 * @brief 좌표별 Matrix 매핑 정보 구조체
 * @details 선 인덱스, 좌표, 시작점 여부, 매트릭스 번호, 표시용 이름 포함
 */
struct CoordinateMatrixMapping {
    int lineIndex;          // 선 인덱스
    QPoint coordinate;      // 좌표
    bool isStartPoint;      // 시작점 여부
    int matrixNum;          // Matrix 번호 (1-4)
    QString displayName;    // 표시용 이름
};

/**
 * @brief QGraphicsView 기반 비디오 뷰어
 * @details 선 그리기, BBox 표시, 저장된 선 데이터 로드 등 제공
 */
class VideoGraphicsView : public QGraphicsView
{
    Q_OBJECT
public:
    /**
     * @brief VideoGraphicsView 생성자
     * @param parent 부모 위젯
     */
    explicit VideoGraphicsView(QWidget *parent = nullptr);
    /**
     * @brief 그리기 모드 설정
     * @param enabled 활성화 여부
     */
    void setDrawingMode(bool enabled);
    /**
     * @brief 모든 선 지우기
     */
    void clearLines();
    /**
     * @brief 선 리스트 반환
     * @return 선 좌표 쌍 리스트
     */
    QList<QPair<QPoint, QPoint>> getLines() const;
    /**
     * @brief 비디오 아이템 반환
     * @return QGraphicsVideoItem 포인터
     */
    QGraphicsVideoItem* getVideoItem() { return m_videoItem; }
    /**
     * @brief 현재 카테고리 설정
     * @param category 카테고리
     */
    void setCurrentCategory(LineCategory category);
    /**
     * @brief 카테고리별 선 리스트 반환
     * @return CategorizedLine 리스트
     */
    QList<CategorizedLine> getCategorizedLines() const;
    /**
     * @brief 카테고리별 선 개수 반환
     * @param category 카테고리
     * @return 선 개수
     */
    int getCategoryLineCount(LineCategory category) const;
    /**
     * @brief 하이라이트 제거
     */
    void clearHighlight();
    /**
     * @brief 저장된 감지선 데이터 화면에 그리기
     * @param detectionLines 감지선 데이터 리스트
     */
    void loadSavedDetectionLines(const QList<DetectionLineData> &detectionLines);
    /**
     * @brief 저장된 도로선 데이터 화면에 그리기
     * @param roadLines 도로선 데이터 리스트
     */
    void loadSavedRoadLines(const QList<RoadLineData> &roadLines);
    /**
     * @brief QGraphicsScene 반환
     * @return QGraphicsScene 포인터
     */
    QGraphicsScene* scene() { return m_scene; }
    /**
     * @brief 즉시 테스트 선 그리기
     */
    void drawImmediateTestLines();
    /**
     * @brief BBox 표시
     * @param bboxes BBox 리스트
     * @param timestamp 타임스탬프
     */
    void setBBoxes(const QList<BBox> &bboxes, qint64 timestamp);
    /**
     * @brief BBox 모두 제거
     */
    void clearBBoxes();
    /**
     * @brief 원본 비디오 크기 설정
     * @param size QSize
     */
    void setOriginalVideoSize(const QSize &size) { m_originalVideoSize = size; }

signals:
    /** @brief 선 그려짐 시그널 */
    void lineDrawn(const QPoint &start, const QPoint &end, LineCategory category);
    /** @brief 좌표 클릭 시그널 */
    void coordinateClicked(int lineIndex, const QPoint &coordinate, bool isStartPoint);

protected:
    /**
     * @brief 마우스 클릭 이벤트 처리
     * @param event 마우스 이벤트
     */
    void mousePressEvent(QMouseEvent *event) override;
    /**
     * @brief 마우스 이동 이벤트 처리
     * @param event 마우스 이벤트
     */
    void mouseMoveEvent(QMouseEvent *event) override;
    /**
     * @brief 마우스 릴리즈 이벤트 처리
     * @param event 마우스 이벤트
     */
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    /** @brief 도로선 하이라이트 */
    void highlightRoadLine(int lineIndex);
    /** @brief 좌표 하이라이트 */
    void highlightCoordinate(int lineIndex, bool isStartPoint);
    /** @brief QGraphicsScene 포인터 */
    QGraphicsScene *m_scene;
    /** @brief QGraphicsVideoItem 포인터 */
    QGraphicsVideoItem *m_videoItem;
    /** @brief 그리기 모드 여부 */
    bool m_drawingMode;
    /** @brief 현재 그리기 중 여부 */
    bool m_drawing;
    /** @brief 시작점 */
    QPoint m_startPoint;
    /** @brief 현재점 */
    QPoint m_currentPoint;
    /** @brief 선 좌표 쌍 리스트 */
    QList<QPair<QPoint, QPoint>> m_lines;
    /** @brief 선 아이템 리스트 */
    QList<QGraphicsLineItem*> m_lineItems;
    /** @brief 점 아이템 리스트 */
    QList<QGraphicsEllipseItem*> m_pointItems;
    /** @brief 현재 선 아이템 */
    QGraphicsLineItem *m_currentLineItem;
    /** @brief 현재 카테고리 */
    LineCategory m_currentCategory;
    /** @brief 카테고리별 선 리스트 */
    QList<CategorizedLine> m_categorizedLines;
    /** @brief BBox 사각형 아이템 리스트 */
    QList<QGraphicsRectItem*> m_bboxRectItems;
    /** @brief BBox 텍스트 아이템 리스트 */
    QList<QGraphicsTextItem*> m_bboxTextItems;
    /** @brief 원본 비디오 크기 */
    QSize m_originalVideoSize;
    /** @brief 현재 뷰 크기 */
    QSize m_currentViewSize;
};

#endif // VIDEOGRAPHICSVIEW_H


