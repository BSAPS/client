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
    void categorizedLinesReady(const QList<RoadLineData> &roadLines, const QList<DetectionLineData> &detectionLines);

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

    CustomTitleBar *titleBar;

private:
    void updateMappingInfo();
    void addCoordinateMapping(int lineIndex, const QPoint &coordinate, bool isStartPoint, int matrixNum);
    void clearCoordinateMappings();
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
