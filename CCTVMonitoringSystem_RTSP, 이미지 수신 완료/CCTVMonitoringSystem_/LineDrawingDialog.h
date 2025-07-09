#ifndef LINEDRAWINGDIALOG_H
#define LINEDRAWINGDIALOG_H

#include <QDialog>
#include <QVideoWidget>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QLabel>
#include <QPushButton>
#include <QPoint>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QWidget>

// 투명한 오버레이 위젯 클래스
class VideoOverlayWidget : public QWidget
{
    Q_OBJECT

public:
    explicit VideoOverlayWidget(QWidget *parent = nullptr);

    void setLine(const QPoint &start, const QPoint &end);
    void clearLine();

signals:
    void mousePressed(const QPoint &pos);
    void mouseMoved(const QPoint &pos);
    void mouseReleased(const QPoint &pos);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    QPoint m_lineStart;
    QPoint m_lineEnd;
    bool m_hasLine;
};

class ClickableVideoWidget : public QVideoWidget
{
    Q_OBJECT

public:
    explicit ClickableVideoWidget(QWidget *parent = nullptr);

    void setLine(const QPoint &start, const QPoint &end);
    void clearLine();

signals:
    void mousePressed(const QPoint &pos);
    void mouseMoved(const QPoint &pos);
    void mouseReleased(const QPoint &pos);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    VideoOverlayWidget *m_overlayWidget;
};

class LineDrawingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LineDrawingDialog(QWidget *parent = nullptr);
    ~LineDrawingDialog();

    void setStreamUrl(const QString &url);

signals:
    void lineDrawn(const QPoint &start, const QPoint &end);

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onVideoMousePressed(const QPoint &pos);
    void onVideoMouseMoved(const QPoint &pos);
    void onVideoMouseReleased(const QPoint &pos);
    void onConfirmClicked();

private:
    void setupUI();
    void setupMediaPlayer();

    ClickableVideoWidget *m_videoWidget;
    QMediaPlayer *m_mediaPlayer;
    QAudioOutput *m_audioOutput;
    QLabel *m_instructionLabel;
    QPushButton *m_confirmButton;
    QPushButton *m_cancelButton;

    QString m_streamUrl;
    QPoint m_startPoint;
    QPoint m_endPoint;
    bool m_isDrawing;
    bool m_lineDrawn;
};

#endif // LINEDRAWINGDIALOG_H
