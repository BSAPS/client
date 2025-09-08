#include "VideoGraphicsView.h"

#include <QDebug>
#include <QGraphicsProxyWidget>

// VideoGraphicsView 구현
VideoGraphicsView::VideoGraphicsView(QWidget *parent)
    : QGraphicsView(parent)
    , m_scene(nullptr)
    , m_videoItem(nullptr)
    , m_drawingMode(false)
    , m_drawing(false)
    , m_currentLineItem(nullptr)
    , m_currentCategory(LineCategory::ROAD_DEFINITION)
    , m_originalVideoSize(3840, 2160)  // 기본 원본 크기 설정
    , m_currentViewSize(960, 540)      // 현재 뷰 크기 설정
{
    // 씬 생성
    m_scene = new QGraphicsScene(this);
    setScene(m_scene);

    // 비디오 아이템 생성
    m_videoItem = new QGraphicsVideoItem();
    m_videoItem->setSize(QSizeF(960, 540));
    m_videoItem->setZValue(-1000); // 비디오를 가장 뒤로 보내기
    m_scene->addItem(m_videoItem);

    // 뷰 설정
    setMinimumSize(960, 540);
    setMaximumSize(960, 540);
    setStyleSheet("background-color: black; border-radius: 8px;");
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // 씬 크기 설정
    m_scene->setSceneRect(0, 0, 960, 540);

    // 뷰 변환 설정 - 1:1 매핑 보장
    resetTransform();
    setTransform(QTransform());

    // 렌더링 힌트 설정
    setRenderHint(QPainter::Antialiasing, true);
    setRenderHint(QPainter::TextAntialiasing, true);

    // 뷰포트 업데이트 모드 설정
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

    // 캐싱 비활성화 (디버깅용)
    setCacheMode(QGraphicsView::CacheNone);

    qDebug() << "VideoGraphicsView 생성됨";
    qDebug() << "씬 크기:" << m_scene->sceneRect();
    qDebug() << "뷰 크기:" << size();
    qDebug() << "비디오 아이템 크기:" << m_videoItem->size();
    qDebug() << "비디오 아이템 Z-Value:" << m_videoItem->zValue();
}

void VideoGraphicsView::setDrawingMode(bool enabled)
{
    m_drawingMode = enabled;
    setCursor(enabled ? Qt::CrossCursor : Qt::ArrowCursor);
    qDebug() << "그리기 모드 변경:" << enabled;
}

void VideoGraphicsView::clearLines()
{
    clearHighlight();

    // 비디오 아이템을 제외한 모든 그래픽 아이템 제거
    QList<QGraphicsItem*> allItems = m_scene->items();
    for (QGraphicsItem* item : allItems) {
        // 비디오 아이템은 제외
        if (item != m_videoItem) {
            m_scene->removeItem(item);
            delete item;
        }
    }

    // 리스트들 초기화
    m_lineItems.clear();
    m_pointItems.clear();
    m_lines.clear();
    m_categorizedLines.clear();

    qDebug() << "모든 선이 지워짐";
}

QList<QPair<QPoint, QPoint>> VideoGraphicsView::getLines() const
{
    return m_lines;
}

void VideoGraphicsView::setCurrentCategory(LineCategory category)
{
    m_currentCategory = category;
    qDebug() << "카테고리 변경:" << (category == LineCategory::ROAD_DEFINITION ? "도로 명시선" : "객체 감지선");
}

QList<CategorizedLine> VideoGraphicsView::getCategorizedLines() const
{
    return m_categorizedLines;
}

int VideoGraphicsView::getCategoryLineCount(LineCategory category) const
{
    int count = 0;
    for (const auto &line : m_categorizedLines) {
        if (line.category == category) {
            count++;
        }
    }
    return count;
}

void VideoGraphicsView::loadSavedRoadLines(const QList<RoadLineData> &roadLines)
{
    qDebug() << "=== loadSavedRoadLines 시작 ===";
    qDebug() << "도로선:" << roadLines.size() << "개";

    // 기존 도로선만 지우기 (비디오 아이템 제외)
    QList<QGraphicsItem*> itemsToRemove;
    QList<QGraphicsItem*> allItems = m_scene->items();
    for (QGraphicsItem* item : allItems) {
        if (item != m_videoItem) {
            itemsToRemove.append(item);
        }
    }
    for (QGraphicsItem* item : itemsToRemove) {
        m_scene->removeItem(item);
        delete item;
    }
    m_lineItems.clear();
    m_pointItems.clear();
    m_lines.clear();
    m_categorizedLines.clear();

    // 도로선 데이터 처리 - 얇은 선으로
    for (int i = 0; i < roadLines.size(); ++i) {
        const auto &roadLine = roadLines[i];
        qDebug() << QString("도로선 %1: index=%2, (%3,%4) → (%5,%6), matrix1=%7, matrix2=%8")
                        .arg(i).arg(roadLine.index)
                        .arg(roadLine.x1).arg(roadLine.y1).arg(roadLine.x2).arg(roadLine.y2)
                        .arg(roadLine.matrixNum1).arg(roadLine.matrixNum2);

        // 좌표 유효성 검사
        if ((roadLine.x1 == 0 && roadLine.y1 == 0 && roadLine.x2 == 0 && roadLine.y2 == 0) ||
            (roadLine.x1 == roadLine.x2 && roadLine.y1 == roadLine.y2)) {
            qDebug() << "도로선" << i << "좌표가 유효하지 않음 - 건너뜀";
            continue;
        }

        int x1 = roadLine.x1;
        int y1 = roadLine.y1;
        int x2 = roadLine.x2;
        int y2 = roadLine.y2;

        CategorizedLine catLine;
        catLine.start = QPoint(x1, y1);
        catLine.end = QPoint(x2, y2);
        catLine.category = LineCategory::ROAD_DEFINITION;
        m_categorizedLines.append(catLine);
        m_lines.append(qMakePair(catLine.start, catLine.end));

        QGraphicsLineItem *lineItem = new QGraphicsLineItem(QLineF(catLine.start, catLine.end));
        QPen linePen(Qt::blue, 2, Qt::SolidLine);
        lineItem->setPen(linePen);
        lineItem->setZValue(1000);
        m_scene->addItem(lineItem);
        m_lineItems.append(lineItem);

        QGraphicsEllipseItem *startPoint = new QGraphicsEllipseItem(x1 - 3, y1 - 3, 6, 6);
        startPoint->setBrush(Qt::blue);
        startPoint->setPen(QPen(Qt::white, 1));
        startPoint->setZValue(1001);
        m_scene->addItem(startPoint);
        m_pointItems.append(startPoint);

        QGraphicsEllipseItem *endPoint = new QGraphicsEllipseItem(x2 - 3, y2 - 3, 6, 6);
        endPoint->setBrush(Qt::blue);
        endPoint->setPen(QPen(Qt::white, 1));
        endPoint->setZValue(1001);
        m_scene->addItem(endPoint);
        m_pointItems.append(endPoint);

        qDebug() << QString("도로선 %1 그리기 완료: (%2,%3) → (%4,%5)")
                        .arg(roadLine.index).arg(x1).arg(y1).arg(x2).arg(y2);

        QPoint startPointQP(x1,y1);
        QPoint endPointQP(x2,y2);
        emit lineDrawn(startPointQP, endPointQP, LineCategory::ROAD_DEFINITION);
    }
    m_scene->update();
    update();
    viewport()->update();
    repaint();
    qDebug() << "=== loadSavedRoadLines 완료 ===";
}

// 감지선만 처리하는 함수
void VideoGraphicsView::loadSavedDetectionLines(const QList<DetectionLineData> &detectionLines)
{
    qDebug() << "=== loadSavedDetectionLines 시작 ===";
    qDebug() << "감지선:" << detectionLines.size() << "개";

    // 감지선 데이터 처리 - 원래 얇은 선으로
    for (int i = 0; i < detectionLines.size(); ++i) {
        const auto &detectionLine = detectionLines[i];
        qDebug() << QString("감지선 %1: index=%2, name=%3, (%4,%5) → (%6,%7), mode=%8")
                        .arg(i).arg(detectionLine.index).arg(detectionLine.name)
                        .arg(detectionLine.x1).arg(detectionLine.y1)
                        .arg(detectionLine.x2).arg(detectionLine.y2)
                        .arg(detectionLine.mode);

        if ((detectionLine.x1 == 0 && detectionLine.y1 == 0 && detectionLine.x2 == 0 && detectionLine.y2 == 0) ||
            (detectionLine.x1 == detectionLine.x2 && detectionLine.y1 == detectionLine.y2)) {
            qDebug() << "감지선" << i << "좌표가 유효하지 않음 - 건너뜀";
            continue;
        }

        int x1 = detectionLine.x1;
        int y1 = detectionLine.y1;
        int x2 = detectionLine.x2;
        int y2 = detectionLine.y2;

        CategorizedLine catLine;
        catLine.start = QPoint(x1, y1);
        catLine.end = QPoint(x2, y2);
        catLine.category = LineCategory::OBJECT_DETECTION;
        m_categorizedLines.append(catLine);
        m_lines.append(qMakePair(catLine.start, catLine.end));

        QGraphicsLineItem *lineItem = new QGraphicsLineItem(QLineF(catLine.start, catLine.end));
        QPen linePen(Qt::red, 2, Qt::SolidLine);
        lineItem->setPen(linePen);
        lineItem->setZValue(1000);
        m_scene->addItem(lineItem);
        m_lineItems.append(lineItem);

        QGraphicsEllipseItem *startPoint = new QGraphicsEllipseItem(x1 - 3, y1 - 3, 6, 6);
        startPoint->setBrush(Qt::red);
        startPoint->setPen(QPen(Qt::white, 1));
        startPoint->setZValue(1001);
        m_scene->addItem(startPoint);
        m_pointItems.append(startPoint);

        QGraphicsEllipseItem *endPoint = new QGraphicsEllipseItem(x2 - 3, y2 - 3, 6, 6);
        endPoint->setBrush(Qt::red);
        endPoint->setPen(QPen(Qt::white, 1));
        endPoint->setZValue(1001);
        m_scene->addItem(endPoint);
        m_pointItems.append(endPoint);

        qDebug() << QString("감지선 %1 그리기 완료: (%2,%3) → (%4,%5)")
                        .arg(detectionLine.index).arg(x1).arg(y1).arg(x2).arg(y2);

        QPoint startPointQP(x1,y1);
        QPoint endPointQP(x2,y2);
        emit lineDrawn(startPointQP, endPointQP, LineCategory::OBJECT_DETECTION);
    }
    m_scene->update();
    update();
    viewport()->update();
    repaint();
    qDebug() << "=== loadSavedDetectionLines 완료 ===";
}

void VideoGraphicsView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        QGraphicsView::mousePressEvent(event);
        return;
    }

    QPointF scenePos = mapToScene(event->pos());
    qDebug() << "마우스 클릭 - 뷰 좌표:" << event->pos() << "씬 좌표:" << scenePos;

    // 그리기 모드가 아닐 때는 도로선의 좌표점 클릭 감지
    if (!m_drawingMode) {
        // 도로선의 시작점과 끝점 클릭 감지
        for (int i = 0; i < m_categorizedLines.size(); ++i) {
            const auto &catLine = m_categorizedLines[i];
            if (catLine.category == LineCategory::ROAD_DEFINITION) {
                // 시작점 클릭 감지 (반경 10픽셀)
                QPointF startPoint(catLine.start);
                if (QLineF(scenePos, startPoint).length() <= 10.0) {
                    highlightCoordinate(i, true);
                    emit coordinateClicked(i, catLine.start, true);
                    return;
                }

                // 끝점 클릭 감지 (반경 10픽셀)
                QPointF endPoint(catLine.end);
                if (QLineF(scenePos, endPoint).length() <= 10.0) {
                    highlightCoordinate(i, false);
                    emit coordinateClicked(i, catLine.end, false);
                    return;
                }
            }
        }
        QGraphicsView::mousePressEvent(event);
        return;
    }

    // 그리기 모드일 때의 기존 로직
    m_startPoint = scenePos.toPoint();
    m_currentPoint = m_startPoint;
    m_drawing = true;

    // 임시 선 생성 (원래 얇은 선)
    m_currentLineItem = new QGraphicsLineItem(QLineF(scenePos, scenePos));
    QPen pen(Qt::yellow, 2, Qt::DashLine); // 원래 얇은 선
    m_currentLineItem->setPen(pen);
    m_currentLineItem->setZValue(2000); // 최고 Z-Value
    m_scene->addItem(m_currentLineItem);

    qDebug() << "선 그리기 시작:" << m_startPoint;
}


void VideoGraphicsView::highlightRoadLine(int lineIndex)
{
    clearHighlight();

    if (lineIndex >= 0 && lineIndex < m_categorizedLines.size()) {
        const auto &catLine = m_categorizedLines[lineIndex];
        if (catLine.category == LineCategory::ROAD_DEFINITION) {
            // 하이라이트 선 생성 (원래 두께)
            QGraphicsLineItem *highlightLine = new QGraphicsLineItem(QLineF(catLine.start, catLine.end));
            QPen highlightPen(Qt::yellow, 4, Qt::SolidLine); // 원래 하이라이트 두께
            highlightLine->setPen(highlightPen);
            highlightLine->setZValue(1500);
            m_scene->addItem(highlightLine);

            // 임시로 저장 (나중에 제거하기 위해)
            highlightLine->setData(0, "highlight");
        }
    }
}

void VideoGraphicsView::highlightCoordinate(int lineIndex, bool isStartPoint)
{
    clearHighlight();

    if (lineIndex >= 0 && lineIndex < m_categorizedLines.size()) {
        const auto &catLine = m_categorizedLines[lineIndex];
        if (catLine.category == LineCategory::ROAD_DEFINITION) {
            QPoint targetPoint = isStartPoint ? catLine.start : catLine.end;

            // 하이라이트 원 생성 (원래 크기)
            QGraphicsEllipseItem *highlightCircle = new QGraphicsEllipseItem(
                targetPoint.x() - 8, targetPoint.y() - 8, 16, 16);
            QPen highlightPen(Qt::yellow, 2, Qt::SolidLine);
            QBrush highlightBrush(Qt::yellow, Qt::SolidPattern);
            highlightCircle->setPen(highlightPen);
            highlightCircle->setBrush(highlightBrush);
            highlightCircle->setZValue(1500);
            m_scene->addItem(highlightCircle);

            // 임시로 저장 (나중에 제거하기 위해)
            highlightCircle->setData(0, "highlight");
        }
    }
}

void VideoGraphicsView::clearHighlight()
{
    // 하이라이트 선들 제거
    QList<QGraphicsItem*> items = m_scene->items();
    for (QGraphicsItem* item : items) {
        if (item->data(0).toString() == "highlight") {
            m_scene->removeItem(item);
            delete item;
        }
    }
}

// BBox 관련 함수 구현
void VideoGraphicsView::setBBoxes(const QList<BBox> &bboxes, qint64 timestamp)
{
    // 기존 BBox 아이템들 제거
    clearBBoxes();

    // 스케일 계산 (원본 해상도 → 뷰어 해상도)
    double scaleX = static_cast<double>(m_currentViewSize.width()) / m_originalVideoSize.width();
    double scaleY = static_cast<double>(m_currentViewSize.height()) / m_originalVideoSize.height();

    for (const BBox &bbox : bboxes) {
        // Vehicle과 Human(Person) 타입만 필터링
        QString lowerType = bbox.type.toLower();
        if (lowerType != "vehical" && lowerType != "person" && lowerType != "human") {
            continue; // 다른 타입은 건너뛰기
        }

        // 좌표 스케일 변환
        QRectF scaledRect(
            bbox.rect.x() * scaleX,
            bbox.rect.y() * scaleY,
            bbox.rect.width() * scaleX,
            bbox.rect.height() * scaleY
            );

        // 사각형 아이템 생성
        QGraphicsRectItem* rectItem = new QGraphicsRectItem(scaledRect);
        QPen pen(Qt::red, 2);
        rectItem->setPen(pen);
        rectItem->setBrush(Qt::NoBrush);
        rectItem->setData(0, "bbox"); // 식별을 위한 데이터 설정
        m_scene->addItem(rectItem);
        m_bboxRectItems.append(rectItem);

        // 텍스트 아이템 생성 (타입과 신뢰도 표시 - 백분율로 표시)
        QString labelText = QString("%1 (%2%)").arg(bbox.type).arg(static_cast<int>(bbox.confidence * 100));
        QGraphicsTextItem* textItem = new QGraphicsTextItem(labelText);

        // 텍스트 스타일 설정
        QFont font = textItem->font();
        font.setPointSize(10);
        font.setBold(true);
        textItem->setFont(font);
        textItem->setDefaultTextColor(Qt::red);

        // 텍스트 위치 설정 (바운딩 박스 위쪽)
        textItem->setPos(scaledRect.x(), scaledRect.y() - 20);
        textItem->setData(0, "bbox_text"); // 식별을 위한 데이터 설정
        m_scene->addItem(textItem);
        m_bboxTextItems.append(textItem);
    }

    qDebug() << QString("[VideoView] BBox 시각화 완료 - %1개 객체, 타임스탬프: %2").arg(bboxes.size()).arg(timestamp);
}

void VideoGraphicsView::clearBBoxes()
{
    // 기존 BBox 사각형 아이템들 제거
    for (QGraphicsRectItem* item : m_bboxRectItems) {
        if (item) {
            m_scene->removeItem(item);
            delete item;
        }
    }
    m_bboxRectItems.clear();

    // 기존 BBox 텍스트 아이템들 제거
    for (QGraphicsTextItem* item : m_bboxTextItems) {
        if (item) {
            m_scene->removeItem(item);
            delete item;
        }
    }
    m_bboxTextItems.clear();

    qDebug() << "[VideoView] BBox 아이템들 제거 완료";
}

void VideoGraphicsView::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_drawingMode || !m_drawing) {
        QGraphicsView::mouseMoveEvent(event);
        return;
    }

    // 뷰 좌표를 씬 좌표로 변환
    QPointF scenePos = mapToScene(event->pos());
    m_currentPoint = scenePos.toPoint();

    // 임시 선 업데이트
    if (m_currentLineItem) {
        m_currentLineItem->setLine(QLineF(m_startPoint, scenePos));
    }
}

void VideoGraphicsView::mouseReleaseEvent(QMouseEvent *event)
{
    if (!m_drawingMode || !m_drawing || event->button() != Qt::LeftButton) {
        QGraphicsView::mouseReleaseEvent(event);
        return;
    }

    m_drawing = false;
    QPointF scenePos = mapToScene(event->pos());
    QPoint endPoint = scenePos.toPoint();

    // 임시 선 제거
    if (m_currentLineItem) {
        m_scene->removeItem(m_currentLineItem);
        delete m_currentLineItem;
        m_currentLineItem = nullptr;
    }

    // 최소 거리 체크
    if ((endPoint - m_startPoint).manhattanLength() > 10) {
        // 카테고리별 색상 설정
        QColor lineColor = (m_currentCategory == LineCategory::ROAD_DEFINITION) ? Qt::blue : Qt::red;

        // 실제 선 추가 (원래 얇은 선)
        QGraphicsLineItem *lineItem = new QGraphicsLineItem(QLineF(m_startPoint, endPoint));
        QPen linePen(lineColor, 2, Qt::SolidLine); // 원래 얇은 선
        lineItem->setPen(linePen);
        lineItem->setZValue(1000);
        m_scene->addItem(lineItem);
        m_lineItems.append(lineItem);

        // 시작점과 끝점에 점 추가 (원래 작은 크기)
        QGraphicsEllipseItem *startPoint = new QGraphicsEllipseItem(m_startPoint.x() - 3, m_startPoint.y() - 3, 6, 6);
        startPoint->setBrush(lineColor);
        startPoint->setPen(QPen(Qt::white, 1));
        startPoint->setZValue(1001);
        m_scene->addItem(startPoint);
        m_pointItems.append(startPoint);

        QGraphicsEllipseItem *endPointItem = new QGraphicsEllipseItem(endPoint.x() - 3, endPoint.y() - 3, 6, 6);
        endPointItem->setBrush(lineColor);
        endPointItem->setPen(QPen(Qt::white, 1));
        endPointItem->setZValue(1001);
        m_scene->addItem(endPointItem);
        m_pointItems.append(endPointItem);

        // 카테고리 정보와 함께 선 저장
        CategorizedLine catLine;
        catLine.start = m_startPoint;
        catLine.end = endPoint;
        catLine.category = m_currentCategory;
        m_categorizedLines.append(catLine);

        // 기존 호환성을 위한 선 정보도 저장
        m_lines.append(qMakePair(m_startPoint, endPoint));

        emit lineDrawn(m_startPoint, endPoint, m_currentCategory);

        QString categoryName = (m_currentCategory == LineCategory::ROAD_DEFINITION) ? "도로 명시선" : "객체 감지선";
        qDebug() << categoryName << "추가됨:" << m_startPoint << "→" << endPoint << "Z-Value:" << lineItem->zValue();
    } else {
        qDebug() << "선이 너무 짧아서 무시됨";
    }
}
