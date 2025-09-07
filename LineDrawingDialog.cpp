#include "LineDrawingDialog.h"
#include "custommessagebox.h"
#include "customtitlebar.h"
#include <QApplication>
#include <QMessageBox>
#include <QDebug>
#include <QUrl>
#include <QTextCursor>
#include <QTextDocument>
#include <QGraphicsProxyWidget>
#include <QInputDialog>
#include <QToolTip>

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

// LineDrawingDialog 구현
LineDrawingDialog::LineDrawingDialog(const QString &rtspUrl, QWidget *parent)
    : QDialog(parent)
    , m_mappingCountLabel(nullptr)
    , m_sendMappingsButton(nullptr)
    , m_buttonLayout(nullptr)
    , m_videoView(nullptr)
    , m_startDrawingButton(nullptr)
    , m_stopDrawingButton(nullptr)
    , m_clearLinesButton(nullptr)
    , m_sendCoordinatesButton(nullptr)
    , m_closeButton(nullptr)
    , m_logTextEdit(nullptr)
    , m_logCountLabel(nullptr)
    , m_clearLogButton(nullptr)
    , m_mediaPlayer(nullptr)
    , m_audioOutput(nullptr)
    , m_rtspUrl(rtspUrl)
    , m_drawnLines()
    , m_isDrawingMode(false)
    , m_currentCategory(LineCategory::ROAD_DEFINITION)
    , m_selectedRoadLineIndex(-1)
    , m_roadLineSelectionMode(false)
    , m_tcpCommunicator(nullptr)
    , m_bboxEnabled(false)
    , m_roadLinesLoaded(false)
    , m_detectionLinesLoaded(false)
{
    setWindowTitle("기준선 그리기");
    setModal(true);
    resize(1200, 700);


    setupUI();
    setupMediaPlayer();

    // 좌표별 클릭 연결
    connect(m_videoView, &VideoGraphicsView::coordinateClicked, this, &LineDrawingDialog::onCoordinateClicked);

    // TCP 통신 설정 및 저장된 선 데이터 요청
    setupTcpConnection();

    // 비디오 스트림 시작
    QTimer::singleShot(500, this, &LineDrawingDialog::startVideoStream);
}

LineDrawingDialog::LineDrawingDialog(const QString &rtspUrl, TcpCommunicator* tcpCommunicator, QWidget *parent)
    : QDialog(parent)
    , m_mappingCountLabel(nullptr)
    , m_sendMappingsButton(nullptr)
    , m_mainLayout(nullptr)
    , m_buttonLayout(nullptr)
    , m_videoView(nullptr)
    , m_startDrawingButton(nullptr)
    , m_stopDrawingButton(nullptr)
    , m_clearLinesButton(nullptr)
    , m_sendCoordinatesButton(nullptr)
    , m_closeButton(nullptr)
    , m_logTextEdit(nullptr)
    , m_logCountLabel(nullptr)
    , m_clearLogButton(nullptr)
    , m_mediaPlayer(nullptr)
    , m_audioOutput(nullptr)
    , m_rtspUrl(rtspUrl)
    , m_drawnLines()
    , m_isDrawingMode(false)
    , m_currentCategory(LineCategory::ROAD_DEFINITION)
    , m_selectedRoadLineIndex(-1)
    , m_roadLineSelectionMode(false)
    , m_tcpCommunicator(tcpCommunicator)
    , m_bboxEnabled(false)
    , m_roadLinesLoaded(false)
    , m_detectionLinesLoaded(false)
{
    setWindowTitle("기준선 그리기");
    setModal(true);
    resize(1200, 700);

    setupUI();
    setupMediaPlayer();

    // 좌표별 클릭 연결
    connect(m_videoView, &VideoGraphicsView::coordinateClicked, this, &LineDrawingDialog::onCoordinateClicked);

    // TCP 통신 설정 및 저장된 선 데이터 요청
    setupTcpConnection();

    // 비디오 스트림 시작
    QTimer::singleShot(500, this, &LineDrawingDialog::startVideoStream);
}

// TCP 통신기 설정 메서드
void LineDrawingDialog::setTcpCommunicator(TcpCommunicator* communicator)
{
    // 기존 연결 해제
    if (m_tcpCommunicator && m_tcpCommunicator != communicator) {
        disconnect(m_tcpCommunicator, &TcpCommunicator::savedRoadLinesReceived,
                  this, &LineDrawingDialog::onSavedRoadLinesReceived);
        disconnect(m_tcpCommunicator, &TcpCommunicator::savedDetectionLinesReceived,
                  this, &LineDrawingDialog::onSavedDetectionLinesReceived);
        disconnect(m_tcpCommunicator, &TcpCommunicator::bboxesReceived,
                  this, &LineDrawingDialog::onBBoxesReceived);
    }

    m_tcpCommunicator = communicator;

    // 새로운 통신기에 시그널 연결
    if (m_tcpCommunicator) {
        connect(m_tcpCommunicator, &TcpCommunicator::savedRoadLinesReceived,
                this, &LineDrawingDialog::onSavedRoadLinesReceived);
        connect(m_tcpCommunicator, &TcpCommunicator::savedDetectionLinesReceived,
                this, &LineDrawingDialog::onSavedDetectionLinesReceived);
        connect(m_tcpCommunicator, &TcpCommunicator::bboxesReceived,
                this, &LineDrawingDialog::onBBoxesReceived);
        
        qDebug() << "LineDrawingDialog에 TcpCommunicator 설정 완료";
    }
}

void LineDrawingDialog::setupTcpConnection()
{
    // TcpCommunicator가 이미 설정되어 있으면 부모 위젯에서 찾지 않음
    if (m_tcpCommunicator) {
        qDebug() << "TcpCommunicator가 이미 설정되어 있습니다.";
    } else {
        // 부모 위젯에서 TcpCommunicator 인스턴스 가져오기 (기존 방식)
        QWidget *currentParent = this->parentWidget();
        while (currentParent && !m_tcpCommunicator) {
            m_tcpCommunicator = currentParent->findChild<TcpCommunicator*>();
            currentParent = currentParent->parentWidget();
        }
    }

    if (m_tcpCommunicator) {
        // 저장된 선 데이터 수신 시그널들 연결
        connect(m_tcpCommunicator, &TcpCommunicator::savedRoadLinesReceived,
                this, &LineDrawingDialog::onSavedRoadLinesReceived);
        connect(m_tcpCommunicator, &TcpCommunicator::savedDetectionLinesReceived,
                this, &LineDrawingDialog::onSavedDetectionLinesReceived);

        // BBox 데이터 수신 시그널 연결
        connect(m_tcpCommunicator, &TcpCommunicator::bboxesReceived,
                this, &LineDrawingDialog::onBBoxesReceived);

        qDebug() << "TCP 통신 설정 완료";
    } else {
        qDebug() << "TcpCommunicator를 찾을 수 없습니다.";
        addLogMessage("서버 연결을 찾을 수 없어 저장된 선을 불러올 수 없습니다.", "WARNING");
    }
}

void LineDrawingDialog::requestSavedLinesFromServer()
{
    if (m_tcpCommunicator && m_tcpCommunicator->isConnectedToServer()) {
        // 도로선과 감지선을 따로 요청
        bool roadSuccess = m_tcpCommunicator->requestSavedRoadLines();
        bool detectionSuccess = m_tcpCommunicator->requestSavedDetectionLines();

        if (roadSuccess && detectionSuccess) {
            addLogMessage("서버에 저장된 도로선과 감지선 데이터를 자동으로 요청", "INFO");

        } else {
            addLogMessage("저장된 선 데이터 요청 실패", "ERROR");
        }
    } else {
        addLogMessage("서버에 연결되어 있지 않아 저장된 선을 불러올 수 없음", "WARNING");
        // 연결이 안 되어 있으면 재시도
        QTimer::singleShot(2000, this, &LineDrawingDialog::requestSavedLinesFromServer);
    }
}

void LineDrawingDialog::onSavedRoadLinesReceived(const QList<RoadLineData> &roadLines)
{
    addLogMessage("=== 도로선 데이터 수신 ===", "SYSTEM");
    qDebug() << "=== onSavedRoadLinesReceived 호출됨 ===";
    qDebug() << "수신된 도로선 개수:" << roadLines.size();

    // 받은 데이터를 모두 출력
    for (int i = 0; i < roadLines.size(); ++i) {
        const auto &roadLine = roadLines[i];
        qDebug() << QString("수신된 도로선 %1: index=%2, x1=%3, y1=%4, x2=%5, y2=%6, matrix1=%7, matrix2=%8")
                        .arg(i).arg(roadLine.index)
                        .arg(roadLine.x1).arg(roadLine.y1).arg(roadLine.x2).arg(roadLine.y2)
                        .arg(roadLine.matrixNum1).arg(roadLine.matrixNum2);

        addLogMessage(QString("도로선 #%1: (%2,%3)→(%4,%5) Matrix:%6,%7")
                          .arg(roadLine.index)
                          .arg(roadLine.x1).arg(roadLine.y1).arg(roadLine.x2).arg(roadLine.y2)
                          .arg(roadLine.matrixNum1).arg(roadLine.matrixNum2), "COORD");
    }

    if (roadLines.isEmpty()) {
        addLogMessage("도로선 데이터 없음", "WARNING");
        m_roadLinesLoaded = true;
        checkAndLoadAllLines();
        return;
    }

    m_loadedRoadLines = roadLines;
    m_roadLinesLoaded = true;

    addLogMessage(QString("도로선 데이터 수신 - 총 %1개").arg(roadLines.size()), "SUCCESS");

    // 도로선이 있는 경우 매핑 정보도 복원
    if (!roadLines.isEmpty()) {
        for (int i = 0; i < roadLines.size(); ++i) {
            const auto &roadLine = roadLines[i];

            // 시작점 매핑 정보 추가
            addCoordinateMapping(i, QPoint(roadLine.x1, roadLine.y1), true, roadLine.matrixNum1);

            // 끝점 매핑 정보 추가
            addCoordinateMapping(i, QPoint(roadLine.x2, roadLine.y2), false, roadLine.matrixNum2);
            qDebug() << "로드시 시작점, 끝점 매핑정보 추가\n";
        }

        updateMappingInfo();
        addLogMessage(QString("도로선 매핑 정보 복원 완료 - %1개 선의 매핑 정보")
                          .arg(roadLines.size()), "ACTION");
    }

    // 모든 선 데이터가 로드되었는지 확인
    checkAndLoadAllLines();
}

void LineDrawingDialog::onSavedDetectionLinesReceived(const QList<DetectionLineData> &detectionLines)
{
    addLogMessage("=== 감지선 데이터 수신 ===", "SYSTEM");
    qDebug() << "=== onSavedDetectionLinesReceived 호출됨 ===";
    qDebug() << "수신된 감지선 개수:" << detectionLines.size();

    // 받은 데이터를 모두 출력
    for (int i = 0; i < detectionLines.size(); ++i) {
        const auto &detectionLine = detectionLines[i];
        qDebug() << QString("수신된 감지선 %1: index=%2, name=%3, x1=%4, y1=%5, x2=%6, y2=%7, mode=%8")
                        .arg(i).arg(detectionLine.index).arg(detectionLine.name)
                        .arg(detectionLine.x1).arg(detectionLine.y1)
                        .arg(detectionLine.x2).arg(detectionLine.y2)
                        .arg(detectionLine.mode);

        addLogMessage(QString("감지선 #%1 (%2): (%3,%4)→(%5,%6) 모드:%7")
                          .arg(detectionLine.index).arg(detectionLine.name)
                          .arg(detectionLine.x1).arg(detectionLine.y1)
                          .arg(detectionLine.x2).arg(detectionLine.y2)
                          .arg(detectionLine.mode), "COORD");
    }

    if (detectionLines.isEmpty()) {
        addLogMessage("감지선 데이터 없음", "WARNING");
        m_detectionLinesLoaded = true;
        checkAndLoadAllLines();
        return;
    }

    m_loadedDetectionLines = detectionLines;
    m_detectionLinesLoaded = true;

    addLogMessage(QString("감지선 데이터 수신 - 총 %1개").arg(detectionLines.size()), "SUCCESS");

    // 모든 선 데이터가 로드되었는지 확인
    checkAndLoadAllLines();
}

void LineDrawingDialog::checkAndLoadAllLines()
{
    // 도로선과 감지선 모두 로드되었을 때 화면에 표시
    if (m_roadLinesLoaded && m_detectionLinesLoaded) {
        addLogMessage(QString("모든 선 데이터 로드 완료 - (도로선:%1개, 감지선:%2개)")
                          .arg(m_loadedRoadLines.size()).arg(m_loadedDetectionLines.size()), "ACTION");

        if (m_videoView) {
            qDebug() << "모든 선 데이터 로드 완료";

            // 즉시 선을 그리기
            m_videoView->loadSavedDetectionLines(m_loadedDetectionLines);
            m_videoView->loadSavedRoadLines(m_loadedRoadLines);
            qDebug() << "비디오 뷰에 저장된 선 데이터 로드 완료";

            addLogMessage("화면에 선 그리기 완료 - 얇은 선으로 표시됨", "SUCCESS");
        }


        // UI 업데이트
        updateCategoryInfo();
        updateButtonStates();

        addLogMessage(QString("모든 저장된 선 데이터가 화면에 표시됨 - 총 %1개")
                          .arg(m_loadedRoadLines.size() + m_loadedDetectionLines.size()), "SUCCESS");

        // 상태 초기화
        m_roadLinesLoaded = false;
        m_detectionLinesLoaded = false;
    } else {
        addLogMessage(QString("데이터 로드 대기 중... (도로선:%1, 감지선:%2)")
                          .arg(m_roadLinesLoaded ? "완료" : "대기")
                          .arg(m_detectionLinesLoaded ? "완료" : "대기"), "INFO");
    }
}

void LineDrawingDialog::onCoordinateClicked(int lineIndex, const QPoint &coordinate, bool isStartPoint)
{
    QString pointType = isStartPoint ? "시작점" : "끝점";
    addLogMessage(QString("도로선 #%1의 %2이 클릭되었습니다: (%3,%4)")
                      .arg(lineIndex + 1)
                      .arg(pointType)
                      .arg(coordinate.x()).arg(coordinate.y()), "ACTION");

    // Dot Matrix 번호 선택 다이얼로그
    QStringList matrixOptions;
    matrixOptions << "Matrix 1" << "Matrix 2" << "Matrix 3" << "Matrix 4";

    bool ok;
    QString selectedMatrix = QInputDialog::getItem(this,
                                                   "Dot Matrix 선택",
                                                   QString("도로선 #%1의 %2\n좌표: (%3,%4)\n\nDot Matrix 번호를 선택하세요:")
                                                       .arg(lineIndex + 1)
                                                       .arg(pointType)
                                                       .arg(coordinate.x()).arg(coordinate.y()),
                                                   matrixOptions, 0, false, &ok);

    if (ok && !selectedMatrix.isEmpty()) {
        // Matrix 번호 추출
        int matrixNum = 1; // 기본값
        if (selectedMatrix.contains("1")) matrixNum = 1;
        else if (selectedMatrix.contains("2")) matrixNum = 2;
        else if (selectedMatrix.contains("3")) matrixNum = 3;
        else if (selectedMatrix.contains("4")) matrixNum = 4;

        // 좌표-Matrix 매핑 저장
        addCoordinateMapping(lineIndex, coordinate, isStartPoint, matrixNum);

        addLogMessage(QString("도로선 #%1 %2 → Matrix %3 매핑 저장됨: (%4,%5)")
                          .arg(lineIndex + 1)
                          .arg(pointType)
                          .arg(matrixNum)
                          .arg(coordinate.x()).arg(coordinate.y()), "SUCCESS");


        updateMappingInfo();

        // 저장 완료 메시지
        CustomMessageBox msgBox(nullptr, "매핑 저장됨",
                                QString("좌표-Matrix 매핑이 저장되었습니다.\n\n"
                                        "• 도로선: #%1 %2\n"
                                        "• Matrix 번호: %3\n"
                                        "• 좌표: (%4,%5)\n\n"
                                        "전송하려면 '좌표 전송' 버튼 클릭")
                                    .arg(lineIndex + 1)
                                    .arg(pointType)
                                    .arg(matrixNum)
                                    .arg(coordinate.x())
                                    .arg(coordinate.y()));
        msgBox.exec();
    } else {
        addLogMessage("Matrix 선택 취소됨", "INFO");
        m_videoView->clearHighlight(); // 하이라이트 제거
    }
}

LineDrawingDialog::~LineDrawingDialog()
{
    stopVideoStream();
    if (m_mediaPlayer) {
        delete m_mediaPlayer;
    }
    if (m_audioOutput) {
        delete m_audioOutput;
    }
}

void LineDrawingDialog::setupUI()
{
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0); // 프레임리스 윈도우를 위해 마진 제거
    m_mainLayout->setSpacing(0); // 레이아웃 간격 제거

    titleBar = new CustomTitleBar(this);
    titleBar->setTitle("Line Drawing Window");

    // 시그널과 슬롯 연결
    connect(titleBar, &CustomTitleBar::minimizeClicked, this, &LineDrawingDialog::showMinimized);
    connect(titleBar, &CustomTitleBar::closeClicked, this, &QDialog::close);

    // 전체 레이아웃 설정
    m_mainLayout->addWidget(titleBar);

    // 컴팩트한 헤더와 카테고리 선택 영역
    QWidget *headerWidget = new QWidget();
    headerWidget->setFixedHeight(80);
    headerWidget->setStyleSheet("background-color: #353B55; padding: 8px;  margin-bottom: 8px;");
    QVBoxLayout *headerLayout = new QVBoxLayout(headerWidget);
    headerLayout->setContentsMargins(10, 5, 10, 5);
    headerLayout->setSpacing(5);

    // 제목과 카테고리를 한 줄에 배치
    QWidget *titleCategoryWidget = new QWidget();
    QHBoxLayout *titleCategoryLayout = new QHBoxLayout(titleCategoryWidget);
    titleCategoryLayout->setContentsMargins(0, 0, 0, 0);
    titleCategoryLayout->setSpacing(20);

    // 제목
    QLabel *titleLabel = new QLabel("기준선 그리기");
    titleLabel->setStyleSheet("color: #ffffff; font-size: 16px; font-weight: bold;");
    titleCategoryLayout->addWidget(titleLabel);

    // 구분선
    QFrame *separator = new QFrame();
    separator->setFrameShape(QFrame::VLine);
    separator->setFrameShadow(QFrame::Sunken);
    separator->setStyleSheet("color: #ccc;");
    titleCategoryLayout->addWidget(separator);


    m_roadLineRadio = new QRadioButton("도로선");
    m_roadLineRadio->setStyleSheet("color: #ffffff; font-size: 12px; font-weight: bold;");
    m_roadLineRadio->setChecked(true);

    m_detectionLineRadio = new QRadioButton("감지선");
    m_detectionLineRadio->setStyleSheet("color: #ffffff; font-size: 12px; font-weight: bold;");

    m_categoryButtonGroup = new QButtonGroup(this);
    m_categoryButtonGroup->addButton(m_roadLineRadio, 0);
    m_categoryButtonGroup->addButton(m_detectionLineRadio, 1);

    connect(m_categoryButtonGroup, &QButtonGroup::idClicked, this, &LineDrawingDialog::onCategoryChanged);

    titleCategoryLayout->addWidget(m_roadLineRadio);
    titleCategoryLayout->addWidget(m_detectionLineRadio);

    titleCategoryLayout->addStretch();

    // 현재 선택 정보
    m_categoryInfoLabel = new QLabel("현재: 도로선");
    m_categoryInfoLabel->setStyleSheet("color: #f37321; font-size: 11px; ");
    titleCategoryLayout->addWidget(m_categoryInfoLabel);

    headerLayout->addWidget(titleCategoryWidget);

    // 통계 정보
    QWidget *statsWidget = new QWidget();
    QHBoxLayout *statsLayout = new QHBoxLayout(statsWidget);
    statsLayout->setContentsMargins(0, 0, 0, 0);
    statsLayout->setSpacing(10);

    m_roadLineCountLabel = new QLabel("도로선: 0개");
    m_roadLineCountLabel->setStyleSheet("color: #ffffff; font-size: 11px; padding: 2px 6px; ");

    m_detectionLineCountLabel = new QLabel("감지선: 0개");
    m_detectionLineCountLabel->setStyleSheet("color: #ffffff; font-size: 11px; padding: 2px 6px; ");

    statsLayout->addWidget(m_roadLineCountLabel);
    statsLayout->addWidget(m_detectionLineCountLabel);

    // 매핑 정보 추가
    m_mappingCountLabel = new QLabel("매핑: 0개");
    m_mappingCountLabel->setStyleSheet("color: #ffffff; font-size: 11px; padding: 2px 6px; ");

    statsLayout->addWidget(m_mappingCountLabel);

    statsLayout->addStretch();

    headerLayout->addWidget(statsWidget);
    m_mainLayout->addWidget(headerWidget);

    // 메인 콘텐츠 영역 (비디오 + 로그)
    QHBoxLayout *contentLayout = new QHBoxLayout();

    // 왼쪽: 비디오 영역
    m_videoView = new VideoGraphicsView(this);
    connect(m_videoView, &VideoGraphicsView::lineDrawn, this, &LineDrawingDialog::onLineDrawn);
    contentLayout->addWidget(m_videoView, 2);

    // 오른쪽: 로그 영역
    QWidget *logContainer = new QWidget();
    logContainer->setMinimumWidth(350);
    logContainer->setMaximumWidth(400);
    logContainer->setStyleSheet("background-color: #353B55; ");

    QVBoxLayout *logLayout = new QVBoxLayout(logContainer);
    logLayout->setContentsMargins(10, 10, 10, 10);
    logLayout->setSpacing(8);

    // 로그 헤더
    QLabel *logHeaderLabel = new QLabel("작업 로그");
    logHeaderLabel->setStyleSheet("color: #ffffff; font-size: 16px; font-weight: bold; padding: 2px;");
    logLayout->addWidget(logHeaderLabel);

    // 로그 카운트 라벨
    m_logCountLabel = new QLabel("로그: 0개");
    m_logCountLabel->setStyleSheet("color: #ffffff; font-size: 12px; padding: 2px;");
    logLayout->addWidget(m_logCountLabel);

    // 로그 텍스트 영역
    m_logTextEdit = new QTextEdit();
    m_logTextEdit->setReadOnly(true);
    m_logTextEdit->setStyleSheet(
        "QTextEdit { "
        "background-color: #666977; "
        "padding: 8px; "
        "font-family: 'Consolas', 'Monaco', monospace; "
        "font-size: 11px; "
        "}"
        );
    logLayout->addWidget(m_logTextEdit);

    // 로그 지우기 버튼
    m_clearLogButton = new QPushButton("로그 지우기");
    m_clearLogButton->setStyleSheet(
        "QPushButton { "
        "background-color: #666977; "
        "color: white; "
        "padding: 8px 15px; "
        "border: none; "
        "border-radius: 4px; "
        "font-weight: bold; font-size:10pt"
        "} "
        "QPushButton:hover { "
        "background-color: #5a6268; "
        "} "
        );
    connect(m_clearLogButton, &QPushButton::clicked, this, &LineDrawingDialog::onClearLogClicked);
    logLayout->addWidget(m_clearLogButton);

    contentLayout->addWidget(logContainer, 1);

    m_mainLayout->addLayout(contentLayout);

    // 버튼 영역
    m_buttonLayout = new QHBoxLayout();

    //저장된 선 불러오기
    QPushButton *loadSavedLinesButton = new QPushButton();
    loadSavedLinesButton->setIcon(QIcon(":/icons/download.png"));
    loadSavedLinesButton->setIconSize(QSize(30,30));
    loadSavedLinesButton->setStyleSheet("QPushButton { background-color: transparent; color: white; font-size: 20px; border: none; padding: 15px 20px;} "
                                          "QPushButton:hover { background-color: rgba(255,255,255,0.1); border-radius: 40px; }");
    loadSavedLinesButton->setToolTip("데이터 불러오기");
    qApp->setStyleSheet("QToolTip { "
                        "color: black; "          // 글씨색
                        "background-color: #ffffff; "  // 밝은 배경색
                        "border: 1px solid gray; "
                        "padding: 3px; "
                        "}");
    connect(loadSavedLinesButton, &QPushButton::clicked, this, &LineDrawingDialog::onLoadSavedLinesClicked);
    m_buttonLayout->addWidget(loadSavedLinesButton);

    //선 그리기
    m_startDrawingButton = new QPushButton();
    m_startDrawingButton->setIcon(QIcon(":/icons/cil_pen.png"));
    m_startDrawingButton->setIconSize(QSize(30,30));
    m_startDrawingButton->setStyleSheet("QPushButton { background-color: transparent; font-size: 20px; border: none; padding: 15px 20px;} "
                                        "QPushButton:hover { background-color: rgba(255,255,255,0.1); border-radius: 40px; }");
    m_startDrawingButton->setToolTip("선 그리기 시작");
    qApp->setStyleSheet("QToolTip { "
                        "color: black; "          // 글씨색
                        "background-color: #ffffff; "  // 밝은 배경색
                        "border: 1px solid gray; "
                        "padding: 3px; "
                        "}");
    connect(m_startDrawingButton, &QPushButton::clicked, this, &LineDrawingDialog::onStartDrawingClicked);
    m_buttonLayout->addWidget(m_startDrawingButton);


    // 선 그리기 중지
    m_stopDrawingButton = new QPushButton();
    m_stopDrawingButton->setIcon(QIcon(":/icons/cil_pen_clicked.png"));
    m_stopDrawingButton->setIconSize(QSize(30,30));
    m_stopDrawingButton->setStyleSheet("QPushButton { background-color: transparent; font-size: 20px; border: none; padding: 15px 20px;} "
                                        "QPushButton:hover { background-color: rgba(255,255,255,0.1); border-radius: 40px; }");
    m_stopDrawingButton->setToolTip("선 그리기 중지");
    qApp->setStyleSheet("QToolTip { "
                        "color: black; "          // 글씨색
                        "background-color: #ffffff; "  // 밝은 배경색
                        "border: 1px solid gray; "
                        "padding: 3px; "
                        "}");
    connect(m_stopDrawingButton, &QPushButton::clicked, this, &LineDrawingDialog::onStopDrawingClicked);
    m_buttonLayout->addWidget(m_stopDrawingButton);
    m_stopDrawingButton->hide();


    //선 지우기
    m_clearLinesButton = new QPushButton();
    m_clearLinesButton->setIcon(QIcon(":/icons/eraser.png"));
    m_clearLinesButton->setIconSize(QSize(30,30));
    m_clearLinesButton->setStyleSheet("QPushButton { background-color: transparent; color: black; font-size: 20px; border: none; padding: 15px 20px;} "
                                        "QPushButton:hover { background-color: rgba(255,255,255,0.1); border-radius: 40px; }");
    m_clearLinesButton->setToolTip("지우기");
    qApp->setStyleSheet("QToolTip { "
                        "color: black; "          // 글씨색
                        "background-color: #ffffff; "  // 밝은 배경색
                        "border: 1px solid gray; "
                        "padding: 3px; "
                        "}");
    connect(m_clearLinesButton, &QPushButton::clicked, this, &LineDrawingDialog::onClearLinesClicked);
    m_buttonLayout->addWidget(m_clearLinesButton);

    //데이터 전송
    m_sendCoordinatesButton = new QPushButton();
    m_sendCoordinatesButton->setIcon(QIcon(":/icons/export-file.png"));
    m_sendCoordinatesButton->setIconSize(QSize(30,30));
    m_sendCoordinatesButton->setStyleSheet("QPushButton { background-color: transparent; color: white; font-size: 20px; border: none; padding: 15px 20px;} "
                                           "QPushButton:hover { background-color: rgba(255,255,255,0.1); border-radius: 40px; }");
    m_sendCoordinatesButton->setToolTip("데이터 전송");
    qApp->setStyleSheet("QToolTip { "
                        "color: black; "          // 글씨색
                        "background-color: #ffffff; "  // 밝은 배경색
                        "border: 1px solid gray; "
                        "padding: 3px; "
                        "}");
    connect(m_sendCoordinatesButton, &QPushButton::clicked, this, &LineDrawingDialog::onSendCoordinatesClicked);
    m_buttonLayout->addWidget(m_sendCoordinatesButton);


    // BBox ON
    m_bboxOnButton = new QPushButton();
    m_bboxOnButton->setIcon(QIcon(":/icons/squares.png"));
    m_bboxOnButton->setIconSize(QSize(30,30));
    m_bboxOnButton->setStyleSheet("QPushButton { background-color: transparent; color: white; font-size: 20px; border: none; padding: 15px 20px;} "
                                   "QPushButton:hover { background-color: rgba(255,255,255,0.1); border-radius: 40px; }");
    m_bboxOnButton->setToolTip("Bounding Box ON");
    qApp->setStyleSheet("QToolTip { "
                        "color: black; "          // 글씨색
                        "background-color: #ffffff; "  // 밝은 배경색
                        "border: 1px solid gray; "
                        "padding: 3px; "
                        "}");
    connect(m_bboxOnButton, &QPushButton::clicked, this, &LineDrawingDialog::onBBoxOnClicked);
    m_buttonLayout->addWidget(m_bboxOnButton);

    // BBox OFF
    m_bboxOffButton = new QPushButton();
    m_bboxOffButton->setIcon(QIcon(":/icons/squares_clicked.png"));
    m_bboxOffButton->setIconSize(QSize(30,30));
    m_bboxOffButton->setStyleSheet("QPushButton { background-color: transparent; color: white; font-size: 20px; border: none; padding: 15px 20px;} "
                                   "QPushButton:hover { background-color: rgba(255,255,255,0.1); border-radius: 40px; }");
    m_bboxOffButton->setToolTip("Bounding Box OFF");
    qApp->setStyleSheet("QToolTip { "
                        "color: black; "          // 글씨색
                        "background-color: #ffffff; "  // 밝은 배경색
                        "border: 1px solid gray; "
                        "padding: 3px; "
                        "}");
    connect(m_bboxOffButton, &QPushButton::clicked, this, &LineDrawingDialog::onBBoxOffClicked);
    m_buttonLayout->addWidget(m_bboxOffButton);
    m_bboxOffButton->hide();

    m_mainLayout->addLayout(m_buttonLayout);

    // 초기 로그 메시지
    addLogMessage("저장된 선 데이터 불러오기", "INFO");


    qDebug() << "UI 설정 완료";
}
void LineDrawingDialog::onStartDrawingClicked()
{
    addLogMessage("감지선을 그리면 수직선이 자동으로 계산되고 전송됨", "INFO");
    m_startDrawingButton->hide();
    m_stopDrawingButton->show();

    m_isDrawingMode = true;
    m_videoView->setDrawingMode(true);

    m_startDrawingButton->setEnabled(false);
    m_stopDrawingButton->setEnabled(true);


    addLogMessage("그리기 모드 활성화", "ACTION");
    updateButtonStates();

    qDebug() << "그리기 모드 활성화";
}

void LineDrawingDialog::setupMediaPlayer()
{
    m_mediaPlayer = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_mediaPlayer->setAudioOutput(m_audioOutput);

    // QGraphicsVideoItem에 비디오 출력 설정
    m_mediaPlayer->setVideoOutput(m_videoView->getVideoItem());

    connect(m_mediaPlayer, &QMediaPlayer::playbackStateChanged, this, &LineDrawingDialog::onPlayerStateChanged);
    connect(m_mediaPlayer, &QMediaPlayer::errorOccurred, this, &LineDrawingDialog::onPlayerError);
    connect(m_mediaPlayer, &QMediaPlayer::mediaStatusChanged, this, &LineDrawingDialog::onMediaStatusChanged);

    qDebug() << "미디어 플레이어 설정 완료";
}

void LineDrawingDialog::startVideoStream()
{
    if (!m_rtspUrl.isEmpty()) {
        qDebug() << "RTSP 스트림 시작:" << m_rtspUrl;
        m_mediaPlayer->setSource(QUrl(m_rtspUrl));
        m_mediaPlayer->play();
    } else {
        qDebug() << "RTSP URL이 비어있습니다.";
    }
}

void LineDrawingDialog::stopVideoStream()
{
    if (m_mediaPlayer) {
        m_mediaPlayer->stop();
    }

}

void LineDrawingDialog::onStopDrawingClicked()
{
    addLogMessage("각 좌표별 Dot Matrix 번호를 설정", "INFO");
    m_startDrawingButton->show();
    m_stopDrawingButton->hide();

    m_isDrawingMode = false;
    m_videoView->setDrawingMode(false);

    m_startDrawingButton->setEnabled(true);
    m_stopDrawingButton->setEnabled(false);

    addLogMessage("그리기 모드 비활성화", "ACTION");
    updateButtonStates();

    qDebug() << "그리기 모드 비활성화";
}

void LineDrawingDialog::onClearLinesClicked()
{
    m_tcpCommunicator->requestDeleteLines();

    int lineCount = m_videoView->getLines().size();
    m_videoView->clearLines();
    m_drawnLines.clear();

    // 매핑 정보도 함께 지우기
    clearCoordinateMappings();

    addLogMessage(QString("%1개의 선과 매핑 정보 삭제").arg(lineCount), "ACTION");
    updateCategoryInfo();
    updateButtonStates();
}


void LineDrawingDialog::onCategoryChanged()
{
    int selectedId = m_categoryButtonGroup->checkedId();
    m_currentCategory = (selectedId == 0) ? LineCategory::ROAD_DEFINITION : LineCategory::OBJECT_DETECTION;

    m_videoView->setCurrentCategory(m_currentCategory);

    if (m_currentCategory == LineCategory::ROAD_DEFINITION) {
        m_categoryInfoLabel->setText("현재: 도로선");
        m_categoryInfoLabel->setStyleSheet("color: #f37321; font-size: 11px; ");
        addLogMessage("도로 명시선 모드로 변경", "ACTION");
    } else {
        m_categoryInfoLabel->setText("현재: 감지선");
        m_categoryInfoLabel->setStyleSheet("color: #f37321; font-size: 11px; ");
        addLogMessage("객체 감지선 모드로 변경", "ACTION");
    }
}

void LineDrawingDialog::onLineDrawn(const QPoint &start, const QPoint &end, LineCategory category)
{
    QString categoryName = (category == LineCategory::ROAD_DEFINITION) ? "도로 명시선" : "객체 감지선";
    addLogMessage(QString("%1 생성 : (%2,%3) → (%4,%5)")
                      .arg(categoryName)
                      .arg(start.x()).arg(start.y())
                      .arg(end.x()).arg(end.y()), "DRAW");


    updateCategoryInfo();
    updateButtonStates();
}

void LineDrawingDialog::updateCategoryInfo()
{
    int roadCount = m_videoView->getCategoryLineCount(LineCategory::ROAD_DEFINITION);
    int detectionCount = m_videoView->getCategoryLineCount(LineCategory::OBJECT_DETECTION);

    m_roadLineCountLabel->setText(QString("도로선: %1개").arg(roadCount));
    m_detectionLineCountLabel->setText(QString("감지선: %1개").arg(detectionCount));
}

void LineDrawingDialog::onSendCoordinatesClicked()
{
    QList<CategorizedLine> allLines = m_videoView->getCategorizedLines();

    if (allLines.isEmpty()) {
        addLogMessage("전송할 선 없음", "WARNING");
        CustomMessageBox msgBox(nullptr, "알림", "전송할 선 없음. 먼저 선을 그려주세요.");
        msgBox.exec();
        return;
    }

    // 매핑 정보가 있는 도로선 우선 처리
    QList<RoadLineData> roadLines = getCoordinateMappingsAsRoadLines();
    QList<DetectionLineData> detectionLines;

    // 매핑 정보가 있는 도로선의 인덱스 저장
    QSet<int> mappedLineIndices;
    for (const auto &mapping : m_coordinateMatrixMappings) {
        if (mapping.lineIndex < allLines.size() &&
            allLines[mapping.lineIndex].category == LineCategory::ROAD_DEFINITION) {
            mappedLineIndices.insert(mapping.lineIndex);
            qDebug() << "매핑 정보가 있는 인덱스 : " << mapping.lineIndex << "\n";
        }
    }

    int autoIndex = roadLines.size() + 1; // 매핑된 선들 다음 인덱스부터 시작

    int matrixNumIndex = roadLines.size();
    for (int i = 0; i < allLines.size(); ++i) {
        const auto &line = allLines[i];

        if (line.category == LineCategory::ROAD_DEFINITION) {
            // 매핑 정보가 없는 도로선만 자동 할당
            if (!mappedLineIndices.contains(i)) {
                RoadLineData roadLineData;
                roadLineData.index = autoIndex++;
                roadLineData.matrixNum1 = matrixNumIndex % 4 + 1;  // 자동 순환 할당
                matrixNumIndex++;
                roadLineData.x1 = line.start.x();
                roadLineData.y1 = line.start.y();
                roadLineData.matrixNum2 = matrixNumIndex % 4 + 1;  // 자동 순환 할당
                matrixNumIndex++;
                roadLineData.x2 = line.end.x();
                roadLineData.y2 = line.end.y();
                roadLines.append(roadLineData);
            }
        } else {
            // 감지선 처리
            DetectionLineData detectionLineData;
            detectionLineData.index = detectionLines.size() + 1;
            detectionLineData.x1 = line.start.x();
            detectionLineData.y1 = line.start.y();
            detectionLineData.x2 = line.end.x();
            detectionLineData.y2 = line.end.y();
            detectionLineData.name = QString("DetectionLine%1").arg(detectionLineData.index);
            detectionLineData.mode = "BothDirections";
            detectionLineData.leftMatrixNum = 1;
            detectionLineData.rightMatrixNum = 2;
            detectionLines.append(detectionLineData);
        }
    }

    int mappedCount = getCoordinateMappingsAsRoadLines().size();
    int autoCount = roadLines.size() - mappedCount;

    addLogMessage(QString("좌표 및 매핑 정보 전송. (매핑된 도로선: %1개, 자동할당 도로선: %2개, 감지선: %3개)")
                      .arg(mappedCount).arg(autoCount).arg(detectionLines.size()), "INFO");

    // 서버 양식에 맞춘 카테고리별 좌표 전송
    emit categorizedLinesReady(roadLines, detectionLines);

    // 로그에 전송될 좌표 정보 출력
    for (const auto &line : roadLines) {
        QString mappingType = (line.index <= mappedCount) ? "매핑" : "자동";
        addLogMessage(QString("도로 기준선 #%1 (%2) (시작점 Matrix:%3, 끝점 Matrix:%4): (%5,%6) → (%7,%8)")
                          .arg(line.index).arg(mappingType)
                          .arg(line.matrixNum1).arg(line.matrixNum2)
                          .arg(line.x1).arg(line.y1)
                          .arg(line.x2).arg(line.y2), "COORD");
    }

    for (const auto &line : detectionLines) {
        addLogMessage(QString("객체 감지선 #%1 (%2, %3): (%4,%5) → (%6,%7)")
                          .arg(line.index).arg(line.name).arg(line.mode)
                          .arg(line.x1).arg(line.y1)
                          .arg(line.x2).arg(line.y2), "COORD");
    }

}

void LineDrawingDialog::onClearLogClicked()
{
    clearLog();
}

void LineDrawingDialog::onPlayerStateChanged(QMediaPlayer::PlaybackState state)
{
    switch (state) {
    case QMediaPlayer::PlayingState:
        addLogMessage("비디오 스트림 연결됨", "SUCCESS");
        break;
    case QMediaPlayer::PausedState:
        break;
    case QMediaPlayer::StoppedState:
        break;
    }
}

void LineDrawingDialog::onPlayerError(QMediaPlayer::Error error, const QString &errorString)
{
    QString errorMsg = QString("비디오 스트림 오류: %1").arg(errorString);
    addLogMessage(errorMsg, "ERROR");
    qDebug() << "미디어 플레이어 오류:" << error << errorString;
}

void LineDrawingDialog::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    switch (status) {
    case QMediaPlayer::LoadingMedia:
        break;
    case QMediaPlayer::LoadedMedia:
        break;
    case QMediaPlayer::BufferingMedia:
        break;
    case QMediaPlayer::BufferedMedia:
        break;
    case QMediaPlayer::EndOfMedia:
        break;
    case QMediaPlayer::InvalidMedia:
        addLogMessage("유효하지 않은 RTSP Stream", "ERROR");
        break;
    default:
        break;
    }
}


void LineDrawingDialog::addLogMessage(const QString &message, const QString &type)
{
    QString timestamp = QDateTime::currentDateTime().toString("[yyyy.MM.dd]" " (hh:mm:ss)");
    QString coloredMessage;

    // 타입별 색상 및 아이콘 설정
    if (type == "ERROR") {
        coloredMessage = QString("<span style='color: #ffffff; font-weight: bold;'> %1 %2</span>").arg(timestamp, message);
    } else if (type == "SUCCESS") {
        coloredMessage = QString("<span style='color: #ffffff; font-weight: bold;'> %1 %2</span>").arg(timestamp, message);
    } else if (type == "WARNING") {
        coloredMessage = QString("<span style='color: #ffffff; font-weight: bold;'>️ %1 %2</span>").arg(timestamp, message);
    } else if (type == "ACTION") {
        coloredMessage = QString("<span style='color: #ffffff; font-weight: bold;'> %1 %2</span>").arg(timestamp, message);
    } else if (type == "DRAW") {
        coloredMessage = QString("<span style='color: #ffffff; font-weight: bold;'>️ %1 %2</span>").arg(timestamp, message);
    } else if (type == "COORD") {
        coloredMessage = QString("<span style='color: #ffffff; font-weight: bold;'> %1 %2</span>").arg(timestamp, message);
    } else if (type == "SYSTEM") {
        coloredMessage = QString("<span style='color: #ffffff; font-weight: bold;'>️ %1 %2</span>").arg(timestamp, message);
    } else {
        coloredMessage = QString("<span style='color: #ffffff; font-weight: bold;'>️ %1 %2</span>").arg(timestamp, message);
    }

    m_logTextEdit->append(coloredMessage);


    // 자동 스크롤
    QTextCursor cursor = m_logTextEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_logTextEdit->setTextCursor(cursor);

    // 로그 카운트 업데이트
    int logCount = m_logTextEdit->document()->blockCount();
    m_logCountLabel->setText(QString("로그: %1개").arg(logCount));
}

void LineDrawingDialog::clearLog()
{
    m_logTextEdit->clear();
    m_logCountLabel->setText("로그: 0개");
    addLogMessage("로그 삭제됨", "SYSTEM");
}

void LineDrawingDialog::updateButtonStates()
{
    bool hasLines = !m_videoView->getLines().isEmpty();
    m_clearLinesButton->setEnabled(hasLines);
    m_sendCoordinatesButton->setEnabled(hasLines);
}


// 좌표별 Matrix 매핑 관련 함수들
void LineDrawingDialog::addCoordinateMapping(int lineIndex, const QPoint &coordinate, bool isStartPoint, int matrixNum)
{
    // 기존 매핑 제거 (같은 선의 같은 점에 대해)
    for (int i = m_coordinateMatrixMappings.size() - 1; i >= 0; --i) {
        if (m_coordinateMatrixMappings[i].lineIndex == lineIndex &&
            m_coordinateMatrixMappings[i].isStartPoint == isStartPoint) {
            m_coordinateMatrixMappings.removeAt(i);
        }
    }

    // 새 매핑 추가
    CoordinateMatrixMapping mapping;
    mapping.lineIndex = lineIndex;
    mapping.coordinate = coordinate;
    mapping.isStartPoint = isStartPoint;
    mapping.matrixNum = matrixNum;
    mapping.displayName = QString("도로선 #%1 %2").arg(lineIndex + 1).arg(isStartPoint ? "시작점" : "끝점");

    m_coordinateMatrixMappings.append(mapping);
}

void LineDrawingDialog::clearCoordinateMappings()
{
    m_coordinateMatrixMappings.clear();
    updateMappingInfo();
}

void LineDrawingDialog::updateMappingInfo()
{
    m_mappingCountLabel->setText(QString("매핑: %1개").arg(m_coordinateMatrixMappings.size()));
}

QList<RoadLineData> LineDrawingDialog::getCoordinateMappingsAsRoadLines() const
{
    QList<RoadLineData> roadLines;
    QMap<int, RoadLineData> lineDataMap;

    // 매핑 정보를 기반으로 RoadLineData 생성
    for (const auto &mapping : m_coordinateMatrixMappings) {
        if (!lineDataMap.contains(mapping.lineIndex)) {
            RoadLineData roadLine;
            roadLine.index = mapping.lineIndex + 1;
            lineDataMap[mapping.lineIndex] = roadLine;
        }

        RoadLineData &roadLine = lineDataMap[mapping.lineIndex];

        if (mapping.isStartPoint) {
            roadLine.x1 = mapping.coordinate.x();
            roadLine.y1 = mapping.coordinate.y();
            roadLine.matrixNum1 = mapping.matrixNum;
        } else {
            roadLine.x2 = mapping.coordinate.x();
            roadLine.y2 = mapping.coordinate.y();
            roadLine.matrixNum2 = mapping.matrixNum;
        }
    }

    // 완전한 매핑 정보를 가진 선들만 반환
    for (auto it = lineDataMap.begin(); it != lineDataMap.end(); ++it) {
        const RoadLineData &roadLine = it.value();
        if (roadLine.x1 != 0 || roadLine.y1 != 0 || roadLine.x2 != 0 || roadLine.y2 != 0) {
            RoadLineData completeRoadLine = roadLine;
            if (completeRoadLine.matrixNum1 == 0) completeRoadLine.matrixNum1 = 1;
            if (completeRoadLine.matrixNum2 == 0) completeRoadLine.matrixNum2 = 2;
            roadLines.append(completeRoadLine);
        }
    }

    // 인덱스 순으로 정렬
    std::sort(roadLines.begin(), roadLines.end(), [](const RoadLineData &a, const RoadLineData &b) {
        return a.index < b.index;
    });

    return roadLines;
}

void LineDrawingDialog::onLoadSavedLinesClicked()
{
    if (!m_tcpCommunicator) {
        addLogMessage("TCP 통신이 설정되지 않음", "ERROR");
        CustomMessageBox msgBox(nullptr, "오류", "서버 연결이 설정되지 않음");
        msgBox.exec();
        return;
    }

    if (!m_tcpCommunicator->isConnectedToServer()) {
        addLogMessage("서버에 연결되어 있지 않음", "ERROR");
        CustomMessageBox msgBox(nullptr, "오류", "서버에 연결되어 있지 않음");
        msgBox.exec();
        return;
    }
    m_tcpCommunicator->setVideoView(m_videoView);

    addLogMessage("수동으로 저장된 선 데이터 요청", "ACTION");

    // 상태 초기화
    m_roadLinesLoaded = false;
    m_detectionLinesLoaded = false;
    m_loadedRoadLines.clear();
    m_loadedDetectionLines.clear();

    // 도로선과 감지선을 따로 요청
    bool roadSuccess = m_tcpCommunicator->requestSavedRoadLines();
    bool detectionSuccess = m_tcpCommunicator->requestSavedDetectionLines();

    // 상단의 함수들이 완료되고 update 함수가 수행될 수 있도록 delay 0.5초
    QEventLoop loop;
    QTimer::singleShot(500, &loop, &QEventLoop::quit);
    loop.exec();


    updateCategoryInfo();
    updateMappingInfo();
    updateButtonStates();

    if (roadSuccess && detectionSuccess) {
        addLogMessage("서버에 저장된 선 데이터 요청", "SUCCESS");
    } else {
        addLogMessage("저장된 선 데이터 요청 실패", "ERROR");
        CustomMessageBox msgBox(nullptr, "오류", "저장된 선 데이터 요청 실패");
        msgBox.exec();
    }
}

// BBox 데이터 수신 슬롯 구현
void LineDrawingDialog::onBBoxesReceived(const QList<BBox> &bboxes, qint64 timestamp)
{
    qDebug() << QString("Bounding Box 데이터 수신 - %1개 객체, 타임스탬프: %2").arg(bboxes.size()).arg(timestamp);
    
    // BBox가 비활성화되어 있다면 처리하지 않음
    if (!m_bboxEnabled) {
        qDebug() << "Bounding Box 비활성화";
        return;
    }
    
    // VideoGraphicsView에 Bounding Box 전달
    if (m_videoView) {
        m_videoView->setBBoxes(bboxes, timestamp);
        
        // 로그 메시지 추가
        if (bboxes.isEmpty()) {

        } else {
            QString objectList;
            int filteredCount = 0;
            for (const BBox &bbox : bboxes) {
                // Vehicle과 Human(Person) 타입만 로그에 표시
                QString lowerType = bbox.type.toLower();
                if (lowerType == "vehicle" || lowerType == "person" || lowerType == "human") {
                    objectList += QString("%1(%2%) ").arg(bbox.type).arg(static_cast<int>(bbox.confidence * 100));
                    filteredCount++;
                }
            }
            if (filteredCount > 0) {

            } else {

            }
        }
    } else {
        qDebug() << "VideoView null - Bounding Box 표시 불가";
        addLogMessage("Bounding Box 표시 실패 - VideoView를 찾을 수 없음", "ERROR");
    }
}

// BBox ON 버튼 클릭 슬롯
void LineDrawingDialog::onBBoxOnClicked()
{
    m_bboxOffButton->show();
    m_bboxOnButton->hide();

    m_bboxEnabled = true;
    m_bboxOnButton->setEnabled(false);
    m_bboxOffButton->setEnabled(true);
    
    addLogMessage("BBox ON - 객체 감지 표시 활성화", "ACTION");
    
    // 서버에 BBox 활성화 요청
    if (m_tcpCommunicator && m_tcpCommunicator->isConnectedToServer()) {
        QJsonObject bboxRequest;
        bboxRequest["request_id"] = 31;  // BBox 활성화
        bboxRequest["bbox_enabled"] = true;
        bool success = m_tcpCommunicator->sendJsonMessage(bboxRequest);
        if (success) {
            qDebug() << "[BBox] ON 요청 전송 성공 (request_id: 31)";
        } else {
            qDebug() << "[BBox] ON 요청 전송 실패";
        }
    }
}

// BBox OFF 버튼 클릭 슬롯
void LineDrawingDialog::onBBoxOffClicked()
{
    m_bboxOffButton->hide();
    m_bboxOnButton->show();

    m_bboxEnabled = false;
    m_bboxOnButton->setEnabled(true);
    m_bboxOffButton->setEnabled(false);
    
    // 현재 표시된 BBox들을 모두 제거
    if (m_videoView) {
        m_videoView->clearBBoxes();
    }
    
    addLogMessage("BBox OFF - 객체 감지 표시 비활성화", "ACTION");
    
    // 서버에 BBox 비활성화 요청
    if (m_tcpCommunicator && m_tcpCommunicator->isConnectedToServer()) {
        QJsonObject bboxRequest;
        bboxRequest["request_id"] = 32;  // BBox 비활성화
        bboxRequest["bbox_enabled"] = false;
        bool success = m_tcpCommunicator->sendJsonMessage(bboxRequest);
        if (success) {
            qDebug() << "[BBox] OFF 요청 전송 성공 (request_id: 32)";
        } else {
            qDebug() << "[BBox] OFF 요청 전송 실패";
        }
    }
}
