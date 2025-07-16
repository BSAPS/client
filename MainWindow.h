#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QDateEdit>
#include <QSpinBox>
#include <QScrollArea>
#include <QNetworkAccessManager>
#include <QTimer>
#include <QDate>
#include <QCalendarWidget>
#include <QDialog>
#include <QMouseEvent>

#include "VideoStreamWidget.h"
#include "TcpCommunicator.h"
#include "ImageViewerDialog.h"
#include "NetworkConfigDialog.h"
#include "LineDrawingDialog.h"

// 클릭 가능한 이미지 라벨 클래스
class ClickableImageLabel : public QLabel
{
    Q_OBJECT

public:
    explicit ClickableImageLabel(QWidget *parent = nullptr);
    void setImageData(const QString &imagePath, const QString &timestamp, const QString &logText);

signals:
    void clicked(const QString &imagePath, const QString &timestamp, const QString &logText);

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    QString m_imagePath;
    QString m_timestamp;
    QString m_logText;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onTabChanged(int index);
    void onWarningButtonClicked();
    void onNetworkConfigClicked();
    void onVideoStreamClicked();
    void onDateChanged(const QDate &date);
    void onHourChanged(int hour);
    void onDateButtonClicked();
    void onCalendarDateSelected(const QDate &date);
    void onHourComboChanged(int index);
    void onStreamingButtonClicked();
    void onRequestImagesClicked();
    void onTcpConnected();
    void onTcpDisconnected();
    void onTcpError(const QString &error);
    void onTcpDataReceived(const QString &data);
    void onTcpPacketReceived(int requestId, int success, const QString &data1, const QString &data2, const QString &data3);
    void onImagesReceived(const QList<ImageData> &images);
    void onImageClicked(const QString &imagePath, const QString &timestamp, const QString &logText);
    void updateLogDisplay();
    void onRequestTimeout();
    void onStreamError(const QString &error);
    void onCoordinatesConfirmed(bool success, const QString &message);
    void onStatusUpdated(const QString &status);

private:
    void setupUI();
    void setupLiveVideoTab();
    void setupCapturedImageTab();
    void setupWarningButtons();
    void setupNetworkConnection();
    void applyStyles();
    QString getWarningButtonStyle(bool isActive);
    void updateWarningButtonStyles();
    void clearImageGrid();
    void displayImages(const QList<ImageData> &images);
    void sendMultipleLineCoordinates(const QList<QPair<QPoint, QPoint>> &lines);
    void sendSingleLineCoordinates(int x1, int y1, int x2, int y2);
    void sendCategorizedCoordinates(const QList<RoadLineData> &roadLines, const QList<DetectionLineData> &detectionLines);

    // UI 컴포넌트
    QWidget *m_centralWidget;
    QTabWidget *m_tabWidget;

    // Live Video Tab
    QWidget *m_liveVideoTab;
    VideoStreamWidget *m_videoStreamWidget;
    QPushButton *m_streamingButton;

    // Captured Image Tab
    QWidget *m_capturedImageTab;
    QScrollArea *m_imageScrollArea;
    QWidget *m_imageGridWidget;
    QGridLayout *m_imageGridLayout;
    QPushButton *m_dateButton;
    QCalendarWidget *m_calendarWidget;
    QDialog *m_calendarDialog;
    QComboBox *m_hourComboBox;
    QDateEdit *m_dateEdit;
    QSpinBox *m_hourSpinBox;
    QPushButton *m_requestButton;
    QLabel *m_statusLabel;

    // 사이드바
    QComboBox *m_modeComboBox;
    QPushButton *m_networkButton;
    QList<QPushButton*> m_warningButtons;

    // 네트워크 설정
    QString m_rtspUrl;
    QString m_tcpHost;
    int m_tcpPort;
    bool m_isConnected;

    // 네트워크 관련
    TcpCommunicator *m_tcpCommunicator;
    QNetworkAccessManager *m_networkManager;
    QTimer *m_updateTimer;
    QTimer *m_requestTimeoutTimer;

    // 다이얼로그
    ImageViewerDialog *m_imageViewerDialog;
    NetworkConfigDialog *m_networkDialog;
    LineDrawingDialog *m_lineDrawingDialog;

    // 상태 관리
    QList<bool> m_warningStates;
    QDate m_selectedDate;
};

#endif // MAINWINDOW_H
