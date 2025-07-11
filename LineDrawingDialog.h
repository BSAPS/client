#ifndef LINEDRAWINGDIALOG_H
#define LINEDRAWINGDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QVideoWidget>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QTimer>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QList>
#include <QPair>
#include <QPoint>

class VideoOverlayWidget : public QWidget
{
    Q_OBJECT

public:
    explicit VideoOverlayWidget(QWidget *parent = nullptr);

    void setDrawingMode(bool enabled);
    void clearLines();
    QList<QPair<QPoint, QPoint>> getLines() const;

signals:
    void lineDrawn(const QPoint &start, const QPoint &end);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    bool m_drawingMode;
    bool m_drawing;
    QPoint m_startPoint;
    QPoint m_currentPoint;
    QList<QPair<QPoint, QPoint>> m_lines;
};

class LineDrawingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LineDrawingDialog(const QString &rtspUrl, QWidget *parent = nullptr);
    ~LineDrawingDialog();

signals:
    void lineCoordinatesReady(int x1, int y1, int x2, int y2);

private slots:
    void onStartDrawingClicked();
    void onStopDrawingClicked();
    void onClearLinesClicked();
    void onSendCoordinatesClicked();
    void onLineDrawn(const QPoint &start, const QPoint &end);
    void onPlayerStateChanged(QMediaPlayer::PlaybackState state);
    void onPlayerError(QMediaPlayer::Error error, const QString &errorString);
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void updateFrameCount();
    void updateButtonStates();

private:
    void setupUI();
    void setupMediaPlayer();
    void startVideoStream();
    void stopVideoStream();

    // UI 컴포넌트
    QVBoxLayout *m_mainLayout;
    QHBoxLayout *m_buttonLayout;
    QVideoWidget *m_videoWidget;
    VideoOverlayWidget *m_overlayWidget;

    QPushButton *m_startDrawingButton;
    QPushButton *m_stopDrawingButton;
    QPushButton *m_clearLinesButton;
    QPushButton *m_sendCoordinatesButton;
    QPushButton *m_closeButton;

    QLabel *m_statusLabel;
    QLabel *m_frameCountLabel;

    // 미디어 관련
    QMediaPlayer *m_mediaPlayer;
    QAudioOutput *m_audioOutput;
    QString m_rtspUrl;

    // 상태 관리
    bool m_isDrawingMode;
    QTimer *m_frameTimer;
    int m_frameCount;

    // 그려진 선들
    QList<QPair<QPoint, QPoint>> m_drawnLines;
};

#endif // LINEDRAWINGDIALOG_H
