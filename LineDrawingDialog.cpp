#include "LineDrawingDialog.h"
#include <QApplication>
#include <QMessageBox>
#include <QDebug>
#include <QUrl>
#include <QTextCursor>
#include <QTextDocument>
#include <QGraphicsProxyWidget>
#include <QInputDialog>

// VideoGraphicsView 구현
VideoGraphicsView::VideoGraphicsView(QWidget *parent)
    : QGraphicsView(parent)
    , m_scene(nullptr)
    , m_videoItem(nullptr)
    , m_drawingMode(false)
    , m_drawing(false)
    , m_currentLineItem(nullptr)
    , m_currentCategory(LineCategory::ROAD_DEFINITION)
{
    // 씬 생성
    m_scene = new QGraphicsScene(this);
    setScene(m_scene);

    // 비디오 아이템 생성
    m_videoItem = new QGraphicsVideoItem();
    m_videoItem->setSize(QSizeF(960, 540));
    m_scene->addItem(m_videoItem);

    // 뷰 설정
    setMinimumSize(960, 540);
    setStyleSheet("background-color: black; border: 2px solid #ddd; border-radius: 8px;");
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // 씬 크기 설정
    setMinimumSize(960, 540);
    setMaximumSize(960, 540);
    m_scene->setSceneRect(0, 0, 960, 540);

    qDebug() << "VideoGraphicsView 생성됨";
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

    // 비디오 아이템을 제외한 모든 그래픽 아이템 제거 (수직선 포함)
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

    qDebug() << "모든 선과 수직선이 지워짐";
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

void VideoGraphicsView::clearCategoryLines(LineCategory category)
{
    // 해당 카테고리의 선들만 제거
    for (int i = m_categorizedLines.size() - 1; i >= 0; --i) {
        if (m_categorizedLines[i].category == category) {
            m_categorizedLines.removeAt(i);
        }
    }

    // 화면에서도 해당 카테고리 선들 제거 (전체 다시 그리기)
    clearLines();
    redrawAllLines();
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

void VideoGraphicsView::redrawAllLines()
{
    // 모든 선을 다시 그리기
    for (const auto &catLine : m_categorizedLines) {
        QGraphicsLineItem *lineItem = new QGraphicsLineItem(QLineF(catLine.start, catLine.end));

        // 카테고리별 색상 설정
        QPen linePen;
        if (catLine.category == LineCategory::ROAD_DEFINITION) {
            linePen = QPen(Qt::blue, 2, Qt::SolidLine);  // 도로선: 파란색
        } else {
            linePen = QPen(Qt::red, 2, Qt::SolidLine);   // 감지선: 빨간색
        }

        lineItem->setPen(linePen);
        m_scene->addItem(lineItem);
        m_lineItems.append(lineItem);

        // 시작점과 끝점에 점 추가
        QColor pointColor = (catLine.category == LineCategory::ROAD_DEFINITION) ? Qt::blue : Qt::red;

        QGraphicsEllipseItem *startPoint = new QGraphicsEllipseItem(catLine.start.x() - 3, catLine.start.y() - 3, 6, 6);
        startPoint->setBrush(pointColor);
        startPoint->setPen(QPen(pointColor));
        m_scene->addItem(startPoint);
        m_pointItems.append(startPoint);

        QGraphicsEllipseItem *endPoint = new QGraphicsEllipseItem(catLine.end.x() - 3, catLine.end.y() - 3, 6, 6);
        endPoint->setBrush(pointColor);
        endPoint->setPen(QPen(pointColor));
        m_scene->addItem(endPoint);
        m_pointItems.append(endPoint);
    }
}

void VideoGraphicsView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        QGraphicsView::mousePressEvent(event);
        return;
    }

    QPointF scenePos = mapToScene(event->pos());

    // 그리기 모드가 아닐 때는 도로선의 좌표점 클릭 감지
    if (!m_drawingMode) {
        // 도로선의 시작점과 끝점 클릭 감지
        for (int i = 0; i < m_categorizedLines.size(); ++i) {
            const auto &catLine = m_categorizedLines[i];
            if (catLine.category == LineCategory::ROAD_DEFINITION) {
                // 시작점 클릭 감지 (반경 15픽셀)
                QPointF startPoint(catLine.start);
                if (QLineF(scenePos, startPoint).length() <= 15.0) {
                    highlightCoordinate(i, true); // 시작점 하이라이트
                    emit coordinateClicked(i, catLine.start, true);
                    return;
                }

                // 끝점 클릭 감지 (반경 15픽셀)
                QPointF endPoint(catLine.end);
                if (QLineF(scenePos, endPoint).length() <= 15.0) {
                    highlightCoordinate(i, false); // 끝점 하이라이트
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

    // 임시 선 생성
    m_currentLineItem = new QGraphicsLineItem(QLineF(scenePos, scenePos));
    QPen pen(Qt::yellow, 2, Qt::DashLine);
    m_currentLineItem->setPen(pen);
    m_scene->addItem(m_currentLineItem);

    qDebug() << "선 그리기 시작:" << m_startPoint;
}

QGraphicsLineItem* VideoGraphicsView::findClickedRoadLine(const QPointF &clickPos)
{
    // 클릭 위치 근처의 도로선 찾기
    for (int i = 0; i < m_lineItems.size(); ++i) {
        QGraphicsLineItem* lineItem = m_lineItems[i];
        if (lineItem) {
            QLineF line = lineItem->line();

            // 점과 선분 사이의 거리 계산
            QPointF lineVec = line.p2() - line.p1();
            QPointF pointVec = clickPos - line.p1();
            qreal lineLength = QPointF::dotProduct(lineVec, lineVec);

            if (lineLength > 0) {
                qreal t = QPointF::dotProduct(pointVec, lineVec) / lineLength;
                t = qMax(0.0, qMin(1.0, t));
                QPointF closestPoint = line.p1() + t * lineVec;
                qreal distance = QLineF(clickPos, closestPoint).length();

                if (distance <= 10.0) {
                    return lineItem;
                }
            }
        }
    }
    return nullptr;
}

void VideoGraphicsView::highlightRoadLine(int lineIndex)
{
    clearHighlight();

    if (lineIndex >= 0 && lineIndex < m_categorizedLines.size()) {
        const auto &catLine = m_categorizedLines[lineIndex];
        if (catLine.category == LineCategory::ROAD_DEFINITION) {
            // 하이라이트 선 생성
            QGraphicsLineItem *highlightLine = new QGraphicsLineItem(QLineF(catLine.start, catLine.end));
            QPen highlightPen(Qt::yellow, 4, Qt::SolidLine);
            highlightLine->setPen(highlightPen);
            highlightLine->setZValue(10); // 다른 선들보다 위에 표시
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

            // 하이라이트 원 생성 (더 크게)
            QGraphicsEllipseItem *highlightCircle = new QGraphicsEllipseItem(
                targetPoint.x() - 8, targetPoint.y() - 8, 16, 16);
            QPen highlightPen(Qt::yellow, 3, Qt::SolidLine);
            QBrush highlightBrush(Qt::yellow, Qt::SolidPattern);
            highlightCircle->setPen(highlightPen);
            highlightCircle->setBrush(highlightBrush);
            highlightCircle->setZValue(10); // 다른 요소들보다 위에 표시
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

        // 실제 선 추가
        QGraphicsLineItem *lineItem = new QGraphicsLineItem(QLineF(m_startPoint, endPoint));
        QPen linePen(lineColor, 2, Qt::SolidLine);
        lineItem->setPen(linePen);
        m_scene->addItem(lineItem);
        m_lineItems.append(lineItem);

        // 시작점과 끝점에 점 추가
        QGraphicsEllipseItem *startPoint = new QGraphicsEllipseItem(m_startPoint.x() - 3, m_startPoint.y() - 3, 6, 6);
        startPoint->setBrush(lineColor);
        startPoint->setPen(QPen(lineColor));
        m_scene->addItem(startPoint);
        m_pointItems.append(startPoint);

        QGraphicsEllipseItem *endPointItem = new QGraphicsEllipseItem(endPoint.x() - 3, endPoint.y() - 3, 6, 6);
        endPointItem->setBrush(lineColor);
        endPointItem->setPen(QPen(lineColor));
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
        qDebug() << categoryName << "추가됨:" << m_startPoint << "→" << endPoint;
    } else {
        qDebug() << "선이 너무 짧아서 무시됨";
    }
}

// LineDrawingDialog 구현
LineDrawingDialog::LineDrawingDialog(const QString &rtspUrl, QWidget *parent)
    : QDialog(parent)
    , m_mappingCountLabel(nullptr)
    , m_clearMappingsButton(nullptr)
    , m_sendMappingsButton(nullptr)
    , m_mainLayout(nullptr)
    , m_buttonLayout(nullptr)
    , m_videoView(nullptr)
    , m_startDrawingButton(nullptr)
    , m_stopDrawingButton(nullptr)
    , m_clearLinesButton(nullptr)
    , m_sendCoordinatesButton(nullptr)
    , m_closeButton(nullptr)
    , m_statusLabel(nullptr)
    , m_frameCountLabel(nullptr)
    , m_logTextEdit(nullptr)
    , m_logCountLabel(nullptr)
    , m_clearLogButton(nullptr)
    , m_mediaPlayer(nullptr)
    , m_audioOutput(nullptr)
    , m_rtspUrl(rtspUrl)
    , m_drawnLines()
    , m_isDrawingMode(false)
    , m_frameTimer(nullptr)
    , m_frameCount(0)
    , m_currentCategory(LineCategory::ROAD_DEFINITION)
    , m_selectedRoadLineIndex(-1)
    , m_roadLineSelectionMode(false)
{
    setWindowTitle("기준선 그리기");
    setModal(true);
    resize(1200, 700);

    setupUI();
    setupMediaPlayer();
    startVideoStream();

    // 좌표별 클릭 연결
    connect(m_videoView, &VideoGraphicsView::coordinateClicked, this, &LineDrawingDialog::onCoordinateClicked);
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
        // Matrix 번호 추출 수정
        int matrixNum = 1; // 기본값
        if (selectedMatrix.contains("1")) matrixNum = 1;
        else if (selectedMatrix.contains("2")) matrixNum = 2;
        else if (selectedMatrix.contains("3")) matrixNum = 3;
        else if (selectedMatrix.contains("4")) matrixNum = 4;

        // 좌표-Matrix 매핑 저장 (즉시 전송하지 않음)
        addCoordinateMapping(lineIndex, coordinate, isStartPoint, matrixNum);

        addLogMessage(QString("도로선 #%1 %2 → Matrix %3 매핑 저장됨: (%4,%5)")
                          .arg(lineIndex + 1)
                          .arg(pointType)
                          .arg(matrixNum)
                          .arg(coordinate.x()).arg(coordinate.y()), "SUCCESS");

        m_statusLabel->setText(QString("좌표 (%1,%2) Matrix %3 매핑이 저장되었습니다.")
                                   .arg(coordinate.x()).arg(coordinate.y()).arg(matrixNum));

        updateMappingInfo();

        // 저장 완료 메시지 (전송하지 않음)
        QMessageBox::information(this, "매핑 저장됨",
                                 QString("좌표-Matrix 매핑이 저장되었습니다.\n\n"
                                         "• 도로선: #%1 %2\n"
                                         "• Matrix 번호: %3\n"
                                         "• 좌표: (%4,%5)\n\n"
                                         "전송하려면 '좌표 전송' 버튼을 클릭하세요.")
                                     .arg(lineIndex + 1)
                                     .arg(pointType)
                                     .arg(matrixNum)
                                     .arg(coordinate.x())
                                     .arg(coordinate.y()));
    } else {
        addLogMessage("Matrix 선택이 취소되었습니다.", "INFO");
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
    m_mainLayout = new QVBoxLayout(this);

    // 컴팩트한 헤더와 카테고리 선택 영역
    QWidget *headerWidget = new QWidget();
    headerWidget->setFixedHeight(80);  // 고정 높이 설정
    headerWidget->setStyleSheet("background-color: #f8f9fa; padding: 8px; border-radius: 6px; margin-bottom: 8px;");
    QVBoxLayout *headerLayout = new QVBoxLayout(headerWidget);
    headerLayout->setContentsMargins(10, 5, 10, 5);
    headerLayout->setSpacing(5);

    // 제목과 카테고리를 한 줄에 배치
    QWidget *titleCategoryWidget = new QWidget();
    QHBoxLayout *titleCategoryLayout = new QHBoxLayout(titleCategoryWidget);
    titleCategoryLayout->setContentsMargins(0, 0, 0, 0);
    titleCategoryLayout->setSpacing(20);

    // 제목
    QLabel *titleLabel = new QLabel("📏 기준선 그리기");
    titleLabel->setStyleSheet("color: #333; font-size: 16px; font-weight: bold;");
    titleCategoryLayout->addWidget(titleLabel);

    // 구분선
    QFrame *separator = new QFrame();
    separator->setFrameShape(QFrame::VLine);
    separator->setFrameShadow(QFrame::Sunken);
    separator->setStyleSheet("color: #ccc;");
    titleCategoryLayout->addWidget(separator);

    // 카테고리 선택 영역 (컴팩트하게)
    QLabel *categoryLabel = new QLabel("선 종류:");
    categoryLabel->setStyleSheet("color: #666; font-size: 12px; font-weight: bold;");
    titleCategoryLayout->addWidget(categoryLabel);

    m_roadLineRadio = new QRadioButton("🛣️ 도로선");
    m_roadLineRadio->setStyleSheet("color: #0066cc; font-size: 12px; font-weight: bold;");
    m_roadLineRadio->setChecked(true);

    m_detectionLineRadio = new QRadioButton("🎯 감지선");
    m_detectionLineRadio->setStyleSheet("color: #cc0000; font-size: 12px; font-weight: bold;");

    m_categoryButtonGroup = new QButtonGroup(this);
    m_categoryButtonGroup->addButton(m_roadLineRadio, 0);
    m_categoryButtonGroup->addButton(m_detectionLineRadio, 1);

    connect(m_categoryButtonGroup, &QButtonGroup::idClicked, this, &LineDrawingDialog::onCategoryChanged);

    titleCategoryLayout->addWidget(m_roadLineRadio);
    titleCategoryLayout->addWidget(m_detectionLineRadio);

    titleCategoryLayout->addStretch();

    // 현재 선택 정보 (작게)
    m_categoryInfoLabel = new QLabel("현재: 도로선 (파란색)");
    m_categoryInfoLabel->setStyleSheet("color: #0066cc; font-size: 11px; font-style: italic;");
    titleCategoryLayout->addWidget(m_categoryInfoLabel);

    headerLayout->addWidget(titleCategoryWidget);

    // 통계 정보 (한 줄로 컴팩트하게)
    QWidget *statsWidget = new QWidget();
    QHBoxLayout *statsLayout = new QHBoxLayout(statsWidget);
    statsLayout->setContentsMargins(0, 0, 0, 0);
    statsLayout->setSpacing(10);

    m_roadLineCountLabel = new QLabel("도로선: 0개");
    m_roadLineCountLabel->setStyleSheet("color: #0066cc; font-size: 11px; padding: 2px 6px; background-color: rgba(0,102,204,0.1); border-radius: 3px;");

    m_detectionLineCountLabel = new QLabel("감지선: 0개");
    m_detectionLineCountLabel->setStyleSheet("color: #cc0000; font-size: 11px; padding: 2px 6px; background-color: rgba(204,0,0,0.1); border-radius: 3px;");

    statsLayout->addWidget(m_roadLineCountLabel);
    statsLayout->addWidget(m_detectionLineCountLabel);

    // 통계 정보 섹션에 매핑 정보 추가
    m_mappingCountLabel = new QLabel("매핑: 0개");
    m_mappingCountLabel->setStyleSheet("color: #28a745; font-size: 11px; padding: 2px 6px; background-color: rgba(40,167,69,0.1); border-radius: 3px;");

    statsLayout->addWidget(m_mappingCountLabel);

    statsLayout->addStretch();

    headerLayout->addWidget(statsWidget);
    m_mainLayout->addWidget(headerWidget);

    // 메인 콘텐츠 영역 (비디오 + 로그)
    QHBoxLayout *contentLayout = new QHBoxLayout();

    // 왼쪽: 비디오 영역 (QGraphicsView 사용)
    m_videoView = new VideoGraphicsView(this);
    connect(m_videoView, &VideoGraphicsView::lineDrawn, this, &LineDrawingDialog::onLineDrawn);
    contentLayout->addWidget(m_videoView, 2);

    // 오른쪽: 로그 영역
    QWidget *logContainer = new QWidget();
    logContainer->setMinimumWidth(350);
    logContainer->setMaximumWidth(400);
    logContainer->setStyleSheet("background-color: #f8f9fa; border: 2px solid #ddd; border-radius: 8px;");

    QVBoxLayout *logLayout = new QVBoxLayout(logContainer);
    logLayout->setContentsMargins(10, 10, 10, 10);
    logLayout->setSpacing(8);

    // 로그 헤더
    QLabel *logHeaderLabel = new QLabel("📋 작업 로그");
    logHeaderLabel->setStyleSheet("color: #333; font-size: 16px; font-weight: bold; padding: 5px;");
    logLayout->addWidget(logHeaderLabel);

    // 로그 카운트 라벨
    m_logCountLabel = new QLabel("로그: 0개");
    m_logCountLabel->setStyleSheet("color: #666; font-size: 12px; padding: 2px;");
    logLayout->addWidget(m_logCountLabel);

    // 로그 텍스트 영역
    m_logTextEdit = new QTextEdit();
    m_logTextEdit->setReadOnly(true);
    m_logTextEdit->setStyleSheet(
        "QTextEdit { "
        "background-color: white; "
        "border: 1px solid #ccc; "
        "border-radius: 5px; "
        "padding: 8px; "
        "font-family: 'Consolas', 'Monaco', monospace; "
        "font-size: 11px; "
        "}"
        );
    logLayout->addWidget(m_logTextEdit);

    // 로그 지우기 버튼
    m_clearLogButton = new QPushButton("🗑️ 로그 지우기");
    m_clearLogButton->setStyleSheet(
        "QPushButton { "
        "background-color: #6c757d; "
        "color: white; "
        "padding: 8px 15px; "
        "border: none; "
        "border-radius: 4px; "
        "font-weight: bold; "
        "} "
        "QPushButton:hover { "
        "background-color: #5a6268; "
        "}"
        );
    connect(m_clearLogButton, &QPushButton::clicked, this, &LineDrawingDialog::onClearLogClicked);
    logLayout->addWidget(m_clearLogButton);

    contentLayout->addWidget(logContainer, 1);

    m_mainLayout->addLayout(contentLayout);

    // 상태 정보
    m_statusLabel = new QLabel("비디오 스트림 연결 중...");
    m_statusLabel->setStyleSheet("color: white; font-weight: bold; padding: 5px;");
    m_mainLayout->addWidget(m_statusLabel);

    m_frameCountLabel = new QLabel("프레임: 0");
    m_frameCountLabel->setStyleSheet("color: gray; padding: 2px;");
    m_mainLayout->addWidget(m_frameCountLabel);

    // 버튼 영역
    m_buttonLayout = new QHBoxLayout();

    m_startDrawingButton = new QPushButton("🖊️ 그리기 시작");
    m_startDrawingButton->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; padding: 10px 20px; border: none; border-radius: 5px; font-weight: bold; } QPushButton:hover { background-color: #45a049; }");
    connect(m_startDrawingButton, &QPushButton::clicked, this, &LineDrawingDialog::onStartDrawingClicked);
    m_buttonLayout->addWidget(m_startDrawingButton);

    m_stopDrawingButton = new QPushButton("⏹️ 그리기 중지");
    m_stopDrawingButton->setStyleSheet("QPushButton { background-color: #f44336; color: white; padding: 10px 20px; border: none; border-radius: 5px; font-weight: bold; } QPushButton:hover { background-color: #da190b; }");
    m_stopDrawingButton->setEnabled(false);
    connect(m_stopDrawingButton, &QPushButton::clicked, this, &LineDrawingDialog::onStopDrawingClicked);
    m_buttonLayout->addWidget(m_stopDrawingButton);

    m_clearLinesButton = new QPushButton("🗑️ 선 지우기");
    m_clearLinesButton->setStyleSheet("QPushButton { background-color: #ff9800; color: white; padding: 10px 20px; border: none; border-radius: 5px; font-weight: bold; } QPushButton:hover { background-color: #e68900; }");
    connect(m_clearLinesButton, &QPushButton::clicked, this, &LineDrawingDialog::onClearLinesClicked);
    m_buttonLayout->addWidget(m_clearLinesButton);

    m_clearMappingsButton = new QPushButton("🗑️ 매핑 지우기");
    m_clearMappingsButton->setStyleSheet("QPushButton { background-color: #dc3545; color: white; padding: 10px 20px; border: none; border-radius: 5px; font-weight: bold; } QPushButton:hover { background-color: #c82333; }");
    m_clearMappingsButton->setEnabled(false);
    connect(m_clearMappingsButton, &QPushButton::clicked, this, &LineDrawingDialog::clearCoordinateMappings);
    m_buttonLayout->addWidget(m_clearMappingsButton);

    m_sendCoordinatesButton = new QPushButton("📤 좌표 전송");
    m_sendCoordinatesButton->setStyleSheet("QPushButton { background-color: #2196F3; color: white; padding: 10px 20px; border: none; border-radius: 5px; font-weight: bold; } QPushButton:hover { background-color: #1976D2; }");
    connect(m_sendCoordinatesButton, &QPushButton::clicked, this, &LineDrawingDialog::onSendCoordinatesClicked);
    m_buttonLayout->addWidget(m_sendCoordinatesButton);

    m_buttonLayout->addStretch();

    m_closeButton = new QPushButton("❌ 닫기");
    m_closeButton->setStyleSheet("QPushButton { background-color: #9E9E9E; color: white; padding: 10px 20px; border: none; border-radius: 5px; font-weight: bold; } QPushButton:hover { background-color: #757575; }");
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::reject);
    m_buttonLayout->addWidget(m_closeButton);

    m_mainLayout->addLayout(m_buttonLayout);

    // 프레임 카운터 타이머
    m_frameTimer = new QTimer(this);
    connect(m_frameTimer, &QTimer::timeout, this, &LineDrawingDialog::updateFrameCount);
    m_frameTimer->start(1000);

    // 초기 로그 메시지 수정
    addLogMessage("기준선 그리기 다이얼로그가 시작되었습니다.", "SYSTEM");
    addLogMessage("💡 도로선(파란색)의 시작점이나 끝점을 클릭하여 각 좌표별로 Dot Matrix 번호를 설정하세요.", "INFO");
    addLogMessage("💡 감지선을 그리면 수직선이 자동으로 계산되고 전송됩니다.", "INFO");
    addLogMessage("💡 '좌표 전송'은 매핑 정보를 우선 사용하고, 없으면 자동 할당합니다.", "INFO");

    qDebug() << "UI 설정 완료";
}

void LineDrawingDialog::setupMediaPlayer()
{
    m_mediaPlayer = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_mediaPlayer->setAudioOutput(m_audioOutput);

    // QGraphicsVideoItem에 비디오 출력 설정
    m_mediaPlayer->setVideoOutput(m_videoView->getVideoItem());

    // 볼륨 설정 (0으로 설정하여 소리 끄기)
    m_audioOutput->setVolume(0.0);

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
        m_statusLabel->setText("비디오 스트림 연결 중...");
    } else {
        m_statusLabel->setText("RTSP URL이 설정되지 않았습니다.");
        qDebug() << "RTSP URL이 비어있습니다.";
    }
}

void LineDrawingDialog::stopVideoStream()
{
    if (m_mediaPlayer) {
        m_mediaPlayer->stop();
    }
    if (m_frameTimer) {
        m_frameTimer->stop();
    }
}

void LineDrawingDialog::onStartDrawingClicked()
{
    m_isDrawingMode = true;
    m_videoView->setDrawingMode(true);

    m_startDrawingButton->setEnabled(false);
    m_stopDrawingButton->setEnabled(true);

    m_statusLabel->setText("그리기 모드 활성화 - 마우스로 선을 그어주세요");
    addLogMessage("그리기 모드가 활성화되었습니다.", "ACTION");
    updateButtonStates();

    qDebug() << "그리기 모드 활성화됨";
}

void LineDrawingDialog::onStopDrawingClicked()
{
    m_isDrawingMode = false;
    m_videoView->setDrawingMode(false);

    m_startDrawingButton->setEnabled(true);
    m_stopDrawingButton->setEnabled(false);

    m_statusLabel->setText("그리기 모드 비활성화");
    addLogMessage("그리기 모드가 비활성화되었습니다.", "ACTION");
    updateButtonStates();

    qDebug() << "그리기 모드 비활성화됨";
}

void LineDrawingDialog::onClearLinesClicked()
{
    int lineCount = m_videoView->getLines().size();
    m_videoView->clearLines();
    m_drawnLines.clear();

    // 매핑 정보도 함께 지우기
    clearCoordinateMappings();

    m_statusLabel->setText("모든 선과 매핑이 지워졌습니다");
    addLogMessage(QString("%1개의 선과 매핑 정보가 지워졌습니다.").arg(lineCount), "ACTION");
    updateCategoryInfo();
    updateButtonStates();
}

void LineDrawingDialog::onCategoryChanged()
{
    int selectedId = m_categoryButtonGroup->checkedId();
    m_currentCategory = (selectedId == 0) ? LineCategory::ROAD_DEFINITION : LineCategory::OBJECT_DETECTION;

    m_videoView->setCurrentCategory(m_currentCategory);

    if (m_currentCategory == LineCategory::ROAD_DEFINITION) {
        m_categoryInfoLabel->setText("현재: 도로선 (파란색)");
        m_categoryInfoLabel->setStyleSheet("color: #0066cc; font-size: 11px; font-style: italic;");
        addLogMessage("도로 명시선 모드로 변경되었습니다.", "ACTION");
    } else {
        m_categoryInfoLabel->setText("현재: 감지선 (빨간색)");
        m_categoryInfoLabel->setStyleSheet("color: #cc0000; font-size: 11px; font-style: italic;");
        addLogMessage("객체 감지선 모드로 변경되었습니다.", "ACTION");
    }
}

void LineDrawingDialog::onClearCategoryClicked()
{
    // 현재 선택된 카테고리의 선들만 지우기
    int beforeCount = m_videoView->getCategoryLineCount(m_currentCategory);
    m_videoView->clearCategoryLines(m_currentCategory);

    QString categoryName = (m_currentCategory == LineCategory::ROAD_DEFINITION) ? "도로 명시선" : "객체 감지선";
    addLogMessage(QString("%1 %2개가 지워졌습니다.").arg(categoryName).arg(beforeCount), "ACTION");

    updateCategoryInfo();
    updateButtonStates();
}

// onLineDrawn 함수 수정 (감지선이 그려졌을 때 수직선 자동 생성)
void LineDrawingDialog::onLineDrawn(const QPoint &start, const QPoint &end, LineCategory category)
{
    QString categoryName = (category == LineCategory::ROAD_DEFINITION) ? "도로 명시선" : "객체 감지선";
    addLogMessage(QString("%1이 그려졌습니다: (%2,%3) → (%4,%5)")
                      .arg(categoryName)
                      .arg(start.x()).arg(start.y())
                      .arg(end.x()).arg(end.y()), "DRAW");

    // 감지선인 경우 수직선 자동 생성
    if (category == LineCategory::OBJECT_DETECTION) {
        QList<CategorizedLine> allLines = m_videoView->getCategorizedLines();
        int detectionLineIndex = 0;

        // 현재 그려진 감지선의 인덱스 찾기
        for (int i = 0; i < allLines.size(); ++i) {
            if (allLines[i].category == LineCategory::OBJECT_DETECTION) {
                detectionLineIndex++;
                if (allLines[i].start == start && allLines[i].end == end) {
                    break;
                }
            }
        }

        // 수직선 생성
        CategorizedLine detectionLine;
        detectionLine.start = start;
        detectionLine.end = end;
        detectionLine.category = category;

        generatePerpendicularLine(detectionLine, detectionLineIndex);
    }

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

// onSendCoordinatesClicked 함수 수정 - 매핑 정보 우선 사용
void LineDrawingDialog::onSendCoordinatesClicked()
{
    QList<CategorizedLine> allLines = m_videoView->getCategorizedLines();

    if (allLines.isEmpty()) {
        addLogMessage("전송할 선이 없습니다.", "WARNING");
        QMessageBox::information(this, "알림", "전송할 선이 없습니다. 먼저 선을 그려주세요.");
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
        }
    }

    int autoIndex = roadLines.size() + 1; // 매핑된 선들 다음 인덱스부터 시작

    for (int i = 0; i < allLines.size(); ++i) {
        const auto &line = allLines[i];

        if (line.category == LineCategory::ROAD_DEFINITION) {
            // 매핑 정보가 없는 도로선만 자동 할당
            if (!mappedLineIndices.contains(i)) {
                RoadLineData roadLineData;
                roadLineData.index = autoIndex++;
                roadLineData.matrixNum1 = ((roadLines.size()) % 4) + 1;  // 자동 순환 할당
                roadLineData.x1 = line.start.x();
                roadLineData.y1 = line.start.y();
                roadLineData.matrixNum2 = ((roadLines.size() + 1) % 4) + 1;  // 자동 순환 할당
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
            detectionLineData.name = QString("detection_line_%1").arg(detectionLineData.index);
            detectionLineData.mode = "BothDirections";
            detectionLineData.leftMatrixNum = 1;
            detectionLineData.rightMatrixNum = 2;
            detectionLines.append(detectionLineData);
        }
    }

    int mappedCount = getCoordinateMappingsAsRoadLines().size();
    int autoCount = roadLines.size() - mappedCount;

    addLogMessage(QString("좌표 전송을 시작합니다. (매핑된 도로선: %1개, 자동할당 도로선: %2개, 감지선: %3개)")
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
        addLogMessage(QString("객체 감지선 %1 (index:%2, name:%3, mode:%4): (%5,%6) → (%7,%8)")
                          .arg(line.index).arg(line.index).arg(line.name).arg(line.mode)
                          .arg(line.x1).arg(line.y1).arg(line.x2).arg(line.y2), "COORD");
    }

    m_statusLabel->setText("좌표 전송 중... 서버 응답을 기다리는 중입니다.");

    addLogMessage(QString("좌표 전송이 시작되었습니다. (총 도로선: %1개, 감지선: %2개)")
                      .arg(roadLines.size()).arg(detectionLines.size()), "SUCCESS");

    // 전송 완료 알림
    QMessageBox::information(this, "전송 시작",
                             QString("기준선 좌표 전송을 시작했습니다.\n"
                                     "• 매핑된 도로선: %1개\n"
                                     "• 자동할당 도로선: %2개\n"
                                     "• 객체 감지선: %3개 (자동 수직선 포함)\n"
                                     "• 도로 기준선 총 %4개")
                                 .arg(mappedCount).arg(autoCount)
                                 .arg(detectionLines.size()).arg(roadLines.size()));
}

void LineDrawingDialog::onPlayerStateChanged(QMediaPlayer::PlaybackState state)
{
    qDebug() << "플레이어 상태 변경:" << state;

    switch (state) {
    case QMediaPlayer::PlayingState:
        m_statusLabel->setText("비디오 스트림 재생 중");
        addLogMessage("비디오 스트림 재생이 시작되었습니다.", "STREAM");
        break;
    case QMediaPlayer::PausedState:
        m_statusLabel->setText("비디오 스트림 일시정지");
        addLogMessage("비디오 스트림이 일시정지되었습니다.", "STREAM");
        break;
    case QMediaPlayer::StoppedState:
        m_statusLabel->setText("비디오 스트림 중지됨");
        addLogMessage("비디오 스트림이 중지되었습니다.", "STREAM");
        break;
    }
    updateButtonStates();
}

void LineDrawingDialog::onPlayerError(QMediaPlayer::Error error, const QString &errorString)
{
    qDebug() << "미디어 플레이어 오류:" << error << errorString;

    m_statusLabel->setText(QString("오류: %1").arg(errorString));
    addLogMessage(QString("스트림 오류: %1").arg(errorString), "ERROR");
    QMessageBox::critical(this, "비디오 스트림 오류",
                          QString("비디오 스트림을 재생하는 중 오류가 발생했습니다:\n%1").arg(errorString));
}

void LineDrawingDialog::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    qDebug() << "미디어 상태 변경:" << status;

    switch (status) {
    case QMediaPlayer::NoMedia:
        m_statusLabel->setText("미디어 없음");
        break;
    case QMediaPlayer::LoadingMedia:
        m_statusLabel->setText("미디어 로딩 중...");
        addLogMessage("미디어를 로딩하고 있습니다...", "STREAM");
        break;
    case QMediaPlayer::LoadedMedia:
        m_statusLabel->setText("미디어 로드됨");
        addLogMessage("미디어 로드가 완료되었습니다.", "STREAM");
        break;
    case QMediaPlayer::StalledMedia:
        m_statusLabel->setText("미디어 버퍼링 중...");
        addLogMessage("미디어 버퍼링 중입니다...", "STREAM");
        break;
    case QMediaPlayer::BufferingMedia:
        m_statusLabel->setText("미디어 버퍼링 중...");
        break;
    case QMediaPlayer::BufferedMedia:
        m_statusLabel->setText("미디어 재생 준비 완료");
        addLogMessage("미디어 재생 준비가 완료되었습니다.", "STREAM");
        break;
    case QMediaPlayer::EndOfMedia:
        m_statusLabel->setText("미디어 재생 종료");
        addLogMessage("미디어 재생이 종료되었습니다.", "STREAM");
        break;
    case QMediaPlayer::InvalidMedia:
        m_statusLabel->setText("잘못된 미디어");
        addLogMessage("잘못된 미디어입니다. URL을 확인해주세요.", "ERROR");
        QMessageBox::warning(this, "잘못된 미디어", "비디오 스트림을 재생할 수 없습니다. URL을 확인해주세요.");
        break;
    }
}

void LineDrawingDialog::updateFrameCount()
{
    if (m_mediaPlayer && m_mediaPlayer->playbackState() == QMediaPlayer::PlayingState) {
        m_frameCount++;
        m_frameCountLabel->setText(QString("프레임: %1").arg(m_frameCount));
    }
}

void LineDrawingDialog::updateButtonStates()
{
    bool isStreaming = (m_mediaPlayer && m_mediaPlayer->playbackState() == QMediaPlayer::PlayingState);
    bool hasLines = !m_videoView->getLines().isEmpty();

    m_startDrawingButton->setEnabled(isStreaming && !m_isDrawingMode);
    m_stopDrawingButton->setEnabled(isStreaming && m_isDrawingMode);
    m_clearLinesButton->setEnabled(hasLines);
    m_sendCoordinatesButton->setEnabled(hasLines);
    m_clearMappingsButton->setEnabled(m_coordinateMatrixMappings.size() > 0);
}

void LineDrawingDialog::addLogMessage(const QString &message, const QString &type)
{
    QString timestamp = QTime::currentTime().toString("hh:mm:ss");
    QString colorCode;
    QString prefix;

    // 로그 타입별 색상과 접두사 설정
    if (type == "ERROR") {
        colorCode = "#dc3545";
        prefix = "❌";
    } else if (type == "WARNING") {
        colorCode = "#ffc107";
        prefix = "⚠️";
    } else if (type == "SUCCESS") {
        colorCode = "#28a745";
        prefix = "✅";
    } else if (type == "ACTION") {
        colorCode = "#007bff";
        prefix = "🔧";
    } else if (type == "DRAW") {
        colorCode = "#6f42c1";
        prefix = "✏️";
    } else if (type == "COORD") {
        colorCode = "#fd7e14";
        prefix = "📍";
    } else if (type == "STREAM") {
        colorCode = "#20c997";
        prefix = "📺";
    } else if (type == "SYSTEM") {
        colorCode = "#6c757d";
        prefix = "⚙️";
    } else {
        colorCode = "#333333";
        prefix = "ℹ️";
    }

    QString formattedMessage = QString(
                                   "<span style='color: %1;'><b>[%2]</b> %3 <span style='color: #666;'>%4</span> - %5</span>")
                                   .arg(colorCode)
                                   .arg(timestamp)
                                   .arg(prefix)
                                   .arg(type)
                                   .arg(message);

    m_logTextEdit->append(formattedMessage);

    // 자동 스크롤
    QTextCursor cursor = m_logTextEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_logTextEdit->setTextCursor(cursor);

    // 로그 카운트 업데이트
    static int logCount = 0;
    logCount++;
    m_logCountLabel->setText(QString("로그: %1개").arg(logCount));

    // 최대 1000줄 제한
    QTextDocument *doc = m_logTextEdit->document();
    if (doc->blockCount() > 1000) {
        QTextCursor cursor(doc);
        cursor.movePosition(QTextCursor::Start);
        cursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor, doc->blockCount() - 1000);
        cursor.removeSelectedText();
    }
}

void LineDrawingDialog::clearLog()
{
    m_logTextEdit->clear();
    m_logCountLabel->setText("로그: 0개");
    addLogMessage("로그가 지워졌습니다.", "SYSTEM");
}

void LineDrawingDialog::onClearLogClicked()
{
    clearLog();
}

void LineDrawingDialog::resizeEvent(QResizeEvent *event)
{
    QDialog::resizeEvent(event);
}

void LineDrawingDialog::addCoordinateMapping(int lineIndex, const QPoint &coordinate, bool isStartPoint, int matrixNum)
{
    // 기존 매핑이 있는지 확인 (같은 좌표에 대한 중복 방지)
    for (int i = 0; i < m_coordinateMatrixMappings.size(); ++i) {
        const auto &mapping = m_coordinateMatrixMappings[i];
        if (mapping.lineIndex == lineIndex &&
            mapping.coordinate == coordinate &&
            mapping.isStartPoint == isStartPoint) {
            // 기존 매핑 업데이트
            m_coordinateMatrixMappings[i].matrixNum = matrixNum;
            m_coordinateMatrixMappings[i].displayName = QString("선#%1 %2 → Matrix%3")
                                                            .arg(lineIndex + 1)
                                                            .arg(isStartPoint ? "시작점" : "끝점")
                                                            .arg(matrixNum);
            addLogMessage(QString("기존 매핑이 업데이트되었습니다: %1").arg(m_coordinateMatrixMappings[i].displayName), "INFO");
            return;
        }
    }

    // 새 매핑 추가
    CoordinateMatrixMapping newMapping;
    newMapping.lineIndex = lineIndex;
    newMapping.coordinate = coordinate;
    newMapping.isStartPoint = isStartPoint;
    newMapping.matrixNum = matrixNum;
    newMapping.displayName = QString("선#%1 %2 → Matrix%3")
                                 .arg(lineIndex + 1)
                                 .arg(isStartPoint ? "시작점" : "끝점")
                                 .arg(matrixNum);

    m_coordinateMatrixMappings.append(newMapping);
    addLogMessage(QString("새 매핑이 추가되었습니다: %1").arg(newMapping.displayName), "INFO");
}

void LineDrawingDialog::updateMappingInfo()
{
    int mappingCount = m_coordinateMatrixMappings.size();
    m_mappingCountLabel->setText(QString("매핑: %1개").arg(mappingCount));

    // 버튼 상태 업데이트
    m_clearMappingsButton->setEnabled(mappingCount > 0);

    // 완전한 매핑 정보 확인 (각 도로선의 시작점과 끝점 모두 매핑되었는지)
    QList<RoadLineData> completeRoadLines = getCoordinateMappingsAsRoadLines();
    if (completeRoadLines.size() > 0) {
        addLogMessage(QString("완전한 매핑: %1개 도로선").arg(completeRoadLines.size()), "SUCCESS");
    }
}

void LineDrawingDialog::clearCoordinateMappings()
{
    int count = m_coordinateMatrixMappings.size();
    m_coordinateMatrixMappings.clear();
    m_videoView->clearHighlight();

    addLogMessage(QString("%1개의 좌표 매핑이 지워졌습니다.").arg(count), "ACTION");
    updateMappingInfo();

    QMessageBox::information(this, "매핑 지우기", QString("%1개의 좌표 매핑이 지워졌습니다.").arg(count));
}

QList<RoadLineData> LineDrawingDialog::getCoordinateMappingsAsRoadLines() const
{
    QList<RoadLineData> roadLines;
    QList<CategorizedLine> allLines = m_videoView->getCategorizedLines();

    // 도로선별로 매핑 정보 그룹화 - const 문제 해결을 위해 값으로 저장
    QMap<int, QPair<CoordinateMatrixMapping, CoordinateMatrixMapping>> lineMapping;
    QMap<int, QPair<bool, bool>> hasMappingFlags; // 시작점, 끝점 매핑 여부

    for (const auto &mapping : m_coordinateMatrixMappings) {
        if (mapping.lineIndex < allLines.size() && allLines[mapping.lineIndex].category == LineCategory::ROAD_DEFINITION) {
            if (!lineMapping.contains(mapping.lineIndex)) {
                lineMapping[mapping.lineIndex] = QPair<CoordinateMatrixMapping, CoordinateMatrixMapping>();
                hasMappingFlags[mapping.lineIndex] = QPair<bool, bool>(false, false);
            }

            if (mapping.isStartPoint) {
                lineMapping[mapping.lineIndex].first = mapping;
                hasMappingFlags[mapping.lineIndex].first = true;
            } else {
                lineMapping[mapping.lineIndex].second = mapping;
                hasMappingFlags[mapping.lineIndex].second = true;
            }
        }
    }

    // 완전한 매핑 정보가 있는 도로선만 RoadLineData로 변환
    for (auto it = lineMapping.begin(); it != lineMapping.end(); ++it) {
        int lineIdx = it.key();
        auto mappingPair = it.value();
        auto flagPair = hasMappingFlags[lineIdx];

        if (flagPair.first && flagPair.second) { // 시작점과 끝점 모두 매핑됨
            RoadLineData roadLineData;
            roadLineData.index = roadLines.size() + 1; // 1부터 시작하  { // 시작점과 끝점 모두 매핑됨
            roadLineData.matrixNum1 = mappingPair.first.matrixNum;
            roadLineData.x1 = mappingPair.first.coordinate.x();
            roadLineData.y1 = mappingPair.first.coordinate.y();
            roadLineData.matrixNum2 = mappingPair.second.matrixNum;
            roadLineData.x2 = mappingPair.second.coordinate.x();
            roadLineData.y2 = mappingPair.second.coordinate.y();

            roadLines.append(roadLineData);
        }
    }

    return roadLines;
}

void LineDrawingDialog::generatePerpendicularLine(const CategorizedLine &detectionLine, int index)
{
    // 수직선 데이터 계산
    PerpendicularLineData perpData = calculatePerpendicularLine(detectionLine.start, detectionLine.end, index);

    // 로그 메시지 추가
    addLogMessage(QString("감지선 #%1에 대한 수직선 생성됨: y = %2x + %3")
                      .arg(index)
                      .arg(perpData.a, 0, 'f', 3)
                      .arg(perpData.b, 0, 'f', 3), "DRAW");

    // 수직선을 화면에 시각적으로 표시
    double midX = (detectionLine.start.x() + detectionLine.end.x()) / 2.0;
    double midY = (detectionLine.start.y() + detectionLine.end.y()) / 2.0;

    // 수직선 표시용 선분 계산 (화면 범위 내에서)
    QPoint perpStart, perpEnd;

    if (abs(perpData.a) > 1000) {
        // 거의 수직선인 경우
        perpStart = QPoint(midX, midY - 30);
        perpEnd = QPoint(midX, midY + 30);
    } else {
        // 일반적인 경우: 중점 기준으로 좌우 30픽셀 범위에서 선분 그리기
        double x1 = midX - 30;
        double y1 = perpData.a * x1 + perpData.b;
        double x2 = midX + 30;
        double y2 = perpData.a * x2 + perpData.b;

        perpStart = QPoint(x1, y1);
        perpEnd = QPoint(x2, y2);
    }

    // 수직선을 노란색 점선으로 표시
    QGraphicsLineItem *perpLineItem = new QGraphicsLineItem(QLineF(perpStart, perpEnd));
    QPen perpPen(Qt::yellow, 2, Qt::DashLine);
    perpLineItem->setPen(perpPen);
    perpLineItem->setZValue(5); // 다른 선들보다 위에 표시
    m_videoView->scene()->addItem(perpLineItem);

    addLogMessage(QString("수직선이 화면에 표시되었습니다 (중점: %.1f, %.1f)")
                      .arg(midX).arg(midY), "INFO");

    // 서버로 수직선 데이터 전송을 위한 시그널 발생
    emit perpendicularLineGenerated(perpData.index, perpData.a, perpData.b);
}

PerpendicularLineData LineDrawingDialog::calculatePerpendicularLine(const QPoint &startPoint, const QPoint &endPoint, int index)
{
    // 감지선의 방향 벡터 계산
    QPoint direction = endPoint - startPoint;

    // 수직 벡터 계산 (90도 회전)
    QPoint perpendicular(-direction.y(), direction.x());

    // 수직선의 길이를 감지선 길이의 절반으로 설정
    double length = sqrt(perpendicular.x() * perpendicular.x() + perpendicular.y() * perpendicular.y());
    if (length > 0) {
        double scale = (sqrt(direction.x() * direction.x() + direction.y() * direction.y()) * 0.5) / length;
        perpendicular = QPoint(perpendicular.x() * scale, perpendicular.y() * scale);
    }

    // 감지선의 중점 계산
    QPoint midPoint = QPoint((startPoint.x() + endPoint.x()) / 2,
                             (startPoint.y() + endPoint.y()) / 2);

    // 수직선 방정식 계산 (y = ax + b 형태)
    double a_result, b_result;

    if (abs(direction.x()) < 0.001) {
        // 원래 선이 거의 수직인 경우 → 수직선은 수평선 (기울기 = 0)
        a_result = 0.0;
        b_result = midPoint.y();  // y = midY (수평선)
    } else if (abs(direction.y()) < 0.001) {
        // 원래 선이 거의 수평인 경우 → 수직선은 수직선
        // 수직선은 y = ax + b로 표현할 수 없으므로 매우 큰 기울기로 근사
        a_result = 999999.0;  // 거의 무한대 기울기
        b_result = midPoint.y() - a_result * midPoint.x();
    } else {
        // 일반적인 경우
        // 원래 선의 기울기: m1 = dy/dx
        // 수직선의 기울기: m2 = -dx/dy (수직 조건)
        a_result = -(double)direction.x() / direction.y();

        // 수직선이 중점 (midX, midY)를 지나므로
        // midY = a * midX + b
        // b = midY - a * midX
        b_result = midPoint.y() - a_result * midPoint.x();
    }

    // 수직선 정보 저장
    PerpendicularLineData perpData;
    perpData.index = index;
    perpData.a = a_result;
    perpData.b = b_result;

    return perpData;
}
