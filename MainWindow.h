#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "VideoStreamWidget.h"
#include "TcpCommunicator.h"
#include "ImageViewerDialog.h"
#include "LineDrawingDialog.h"

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

class CustomTitleBar;

/**
 * @brief 클릭 가능한 이미지 라벨 클래스
 * @details 이미지를 클릭할 수 있는 QLabel 확장 클래스입니다.
 */
class ClickableImageLabel : public QLabel
{
    Q_OBJECT

public:
    /**
     * @brief ClickableImageLabel 생성자
     * @param parent 부모 위젯
     * @details 클릭 가능한 이미지 라벨을 생성합니다.
     */
    explicit ClickableImageLabel(QWidget *parent = nullptr);

    /**
     * @brief 이미지 데이터 설정
     * @param imagePath 이미지 경로
     * @param timestamp 타임스탬프
     * @param logText 로그 텍스트
     * @details 이미지와 관련된 데이터를 설정합니다.
     */
    void setImageData(const QString &imagePath, const QString &timestamp, const QString &logText);

signals:
    /**
     * @brief 이미지 클릭 시 시그널
     * @param imagePath 이미지 경로
     * @param timestamp 타임스탬프
     * @param logText 로그 텍스트
     */
    void clicked(const QString &imagePath, const QString &timestamp, const QString &logText);


private:
    /** @brief 이미지 경로 */
    QString m_imagePath;
    /** @brief 타임스탬프 */
    QString m_timestamp;
    /** @brief 로그 텍스트 */
    QString m_logText;

protected:
    /**
     * @brief 마우스 클릭 이벤트 처리
     * @param event 마우스 이벤트
     */
    void mousePressEvent(QMouseEvent *event) override;
};

/**
 * @brief 메인 윈도우 클래스
 * @details CCTV 모니터링 메인 윈도우를 담당합니다.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:

    /**
     * @brief MainWindow 생성자
     * @param parent 부모 위젯
     */
    MainWindow(QWidget *parent = nullptr);

    /**
     * @brief MainWindow 소멸자
     */
    ~MainWindow();

    // TCP 통신기 설정 메서드
    /**
     * @brief TCP 통신기 설정
     * @param communicator TCP 통신 객체
     */
    void setTcpCommunicator(TcpCommunicator* communicator);

private slots:
    /**
     * @brief 비디오 스트림 클릭 시 슬롯
     */
    void onVideoStreamClicked();
    /**
     * @brief 날짜 변경 시 슬롯
     * @param date 변경된 날짜
     */
    void onDateChanged(const QDate &date);
    /**
     * @brief 시간 변경 시 슬롯
     * @param hour 변경된 시간
     */
    void onHourChanged(int hour);
    /**
     * @brief 시간 콤보박스 변경 시 슬롯
     * @param index 인덱스
     */
    void onHourComboChanged(int index);
    /**
     * @brief 스트리밍 버튼 클릭 시 슬롯
     */
    void onStreamingButtonClicked();
    /**
     * @brief 드로우 버튼 클릭 시 슬롯
     */
    void onDrawButtonClicked();
    /**
     * @brief 이미지 요청 버튼 클릭 시 슬롯
     */
    void onRequestImagesClicked();
    /**
     * @brief TCP 연결 성공 시 슬롯
     */
    void onTcpConnected();
    /**
     * @brief TCP 연결 해제 시 슬롯
     */
    void onTcpDisconnected();
    /**
     * @brief TCP 에러 발생 시 슬롯
     * @param error 에러 메시지
     */
    void onTcpError(const QString &error);
    /**
     * @brief TCP 데이터 수신 시 슬롯
     * @param data 수신 데이터
     */
    void onTcpDataReceived(const QString &data);
    /**
     * @brief TCP 패킷 수신 시 슬롯
     * @param requestId 요청 ID
     * @param success 성공 여부
     * @param data1 데이터1
     * @param data2 데이터2
     * @param data3 데이터3
     */
    void onTcpPacketReceived(int requestId, int success, const QString &data1, const QString &data2, const QString &data3);
    /**
     * @brief 이미지 리스트 수신 시 슬롯
     * @param images 이미지 리스트
     */
    void onImagesReceived(const QList<ImageData> &images);
    /**
     * @brief 이미지 클릭 시 슬롯
     * @param imagePath 이미지 경로
     * @param timestamp 타임스탬프
     * @param logText 로그 텍스트
     */
    void onImageClicked(const QString &imagePath, const QString &timestamp, const QString &logText);
    /**
     * @brief 요청 타임아웃 시 슬롯
     */
    void onRequestTimeout();
    /**
     * @brief 스트림 에러 발생 시 슬롯
     * @param error 에러 메시지
     */
    void onStreamError(const QString &error);
    /**
     * @brief 좌표 확인 결과 슬롯
     * @param success 성공 여부
     * @param message 메시지
     */
    void onCoordinatesConfirmed(bool success, const QString &message);
    /**
     * @brief 상태 업데이트 슬롯
     * @param status 상태 메시지
     */
    void onStatusUpdated(const QString &status);

private:
    /**
     * @brief UI 설정
     */
    void setupUI();
    /**
     * @brief 라이브 비디오 탭 설정
     */
    void setupLiveVideoTab();
    /**
     * @brief 캡처 이미지 탭 설정
     */
    void setupCapturedImageTab();
    /**
     * @brief 경고 버튼 설정
     */
    void setupWarningButtons();
    /**
     * @brief 네트워크 연결 설정
     */
    void setupNetworkConnection();
    /**
     * @brief 스타일 적용
     */
    void applyStyles();
    /**
     * @brief 경고 버튼 스타일 반환
     * @param isActive 활성화 여부
     * @return 스타일 문자열
     */
    QString getWarningButtonStyle(bool isActive);
    /**
     * @brief 경고 버튼 스타일 업데이트
     */
    void updateWarningButtonStyles();
    /**
     * @brief 이미지 그리드 초기화
     */
    void clearImageGrid();
    /**
     * @brief 이미지 표시
     * @param images 이미지 리스트
     */
    void displayImages(const QList<ImageData> &images);
    /**
     * @brief 좌표 데이터 전송
     * @param roadLines 도로선 리스트
     * @param detectionLines 감지선 리스트
     */
    void sendCategorizedCoordinates(const QList<RoadLineData> &roadLines, const QList<DetectionLineData> &detectionLines);


    /** @brief 드래그 위치 */
    QPoint m_dragPosition;
    /** @brief 드래그 중 여부 */
    bool m_dragging = false;

    // UI 컴포넌트
    /** @brief 중앙 위젯 */
    QWidget *m_centralWidget;
    /** @brief 탭 위젯 */
    QTabWidget *m_tabWidget;


    // Live Video Tab
    /** @brief 라이브 비디오 탭 */
    QWidget *m_liveVideoTab;
    /** @brief 비디오 스트림 위젯 */
    VideoStreamWidget *m_videoStreamWidget;
    /** @brief 스트리밍 버튼 */
    QPushButton *m_streamingButton;

    // Captured Image Tab
    /** @brief 캡처 이미지 탭 */
    QWidget *m_capturedImageTab;
    /** @brief 이미지 스크롤 영역 */
    QScrollArea *m_imageScrollArea;
    /** @brief 이미지 그리드 위젯 */
    QWidget *m_imageGridWidget;
    /** @brief 이미지 그리드 레이아웃 */
    QGridLayout *m_imageGridLayout;
    /** @brief 날짜 버튼 */
    QPushButton *m_dateButton;
    /** @brief 시간 콤보박스 */
    QComboBox *m_hourComboBox;
    /** @brief 날짜 에디트 */
    QDateEdit *m_dateEdit;
    /** @brief 시간 스핀박스 */
    QSpinBox *m_hourSpinBox;
    /** @brief 이미지 요청 버튼 */
    QPushButton *m_requestButton;
    /** @brief 상태 라벨 */
    QLabel *m_statusLabel;

    // 사이드바
    /** @brief 모드 콤보박스 */
    QComboBox *m_modeComboBox;
    /** @brief 닫기 버튼 */
    QPushButton *m_closeButton;

    // 네트워크 설정
    /** @brief RTSP URL */
    QString m_rtspUrl;
    /** @brief TCP 호스트 */
    QString m_tcpHost;
    /** @brief TCP 포트 */
    int m_tcpPort;
    /** @brief 연결 여부 */
    bool m_isConnected;

    // 네트워크 관련
    /** @brief TCP 통신 객체 */
    TcpCommunicator *m_tcpCommunicator;
    /** @brief 네트워크 매니저 */
    QNetworkAccessManager *m_networkManager;
    /** @brief 업데이트 타이머 */
    QTimer *m_updateTimer;
    /** @brief 이미지 요청 타임아웃 타이머 */
    QTimer *m_requestTimeoutTimer;

    /** @brief 이미지 뷰어 다이얼로그 */
    ImageViewerDialog *m_imageViewerDialog;
    /** @brief 라인 드로잉 다이얼로그 */
    LineDrawingDialog *m_lineDrawingDialog;

    /** @brief 경고 상태 리스트 */
    QList<bool> m_warningStates;
    /** @brief 선택된 날짜 */
    QDate m_selectedDate;

    /** @brief 커스텀 타이틀바 */
    CustomTitleBar *titleBar;
};

#endif // MAINWINDOW_H
