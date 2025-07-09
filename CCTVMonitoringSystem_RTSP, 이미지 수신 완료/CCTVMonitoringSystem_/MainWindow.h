#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QTabWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QDateEdit>
#include <QSpinBox>
#include <QScrollArea>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QMouseEvent>
#include <QList>
#include <QDate>
#include <QTime>
#include <QCalendarWidget>
#include <QDialog>

#include "VideoStreamWidget.h"
#include "TcpCommunicator.h"
#include "ImageViewerDialog.h"
#include "NetworkConfigDialog.h"
#include "LineDrawingDialog.h"

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
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onTabChanged(int index);
    void onWarningButtonClicked();
    void onNetworkConfigClicked();
    void onVideoStreamClicked();
    void onDateChanged(const QDate &date); // 호환성 유지용
    void onHourChanged(int hour); // 호환성 유지용
    void onDateButtonClicked();
    void onCalendarDateSelected(const QDate &date);
    void onHourComboChanged(int index);
    void onStreamingButtonClicked();
    void onRequestImagesClicked();
    void sendLineCoordinates(const QPoint &start, const QPoint &end);
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
    QPushButton *m_dateButton;  // 날짜 선택 버튼
    QCalendarWidget *m_calendarWidget;  // 달력 위젯
    QDialog *m_calendarDialog;  // 달력을 담을 다이얼로그
    QDate m_selectedDate;  // 선택된 날짜
    QComboBox *m_hourComboBox;  // QSpinBox에서 QComboBox로 변경
    QDateEdit *m_dateEdit;      // 호환성 유지용 (사용하지 않음)
    QSpinBox *m_hourSpinBox;    // 호환성 유지용 (사용하지 않음)
    QPushButton *m_requestButton;
    QLabel *m_statusLabel;

    // Sidebar
    QComboBox *m_modeComboBox;
    QPushButton *m_networkButton;
    QList<QPushButton*> m_warningButtons;
    QList<bool> m_warningStates;

    // 네트워크 설정
    QString m_rtspUrl;
    QString m_tcpHost;
    int m_tcpPort;
    bool m_isConnected;

    // 네트워크 컴포넌트
    TcpCommunicator *m_tcpCommunicator;
    QNetworkAccessManager *m_networkManager;
    QTimer *m_updateTimer;
    QTimer *m_requestTimeoutTimer;

    // 다이얼로그
    ImageViewerDialog *m_imageViewerDialog;
    NetworkConfigDialog *m_networkDialog;
    LineDrawingDialog *m_lineDrawingDialog;
};

#endif // MAINWINDOW_H
