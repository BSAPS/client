#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// Qt 기본 헤더 파일들
#include <QMainWindow>          // 메인 윈도우 클래스
#include <QMap>                 // 맵 컨테이너

// 사용자 정의 헤더 파일들
#include "networkclient.h"      // 네트워크 클라이언트
#include "videostreamer.h"      // 비디오 스트리머
#include "imageviewer.h"        // 이미지 뷰어

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);  // 생성자
    ~MainWindow();                          // 소멸자

private slots:
    // 네트워크 관련 슬롯들
    void onConnectClicked();                // 연결 버튼 클릭
    void onDisconnectClicked();             // 연결 해제 버튼 클릭
    void onNetworkConnected();              // 네트워크 연결 완료
    void onNetworkDisconnected();           // 네트워크 연결 해제
    void onNetworkError(const QString &error);  // 네트워크 에러

    // 데이터 수신 슬롯들
    void onVideoFrameReceived(const QByteArray &frameData);     // 비디오 프레임 수신
    void onCapturedImageReceived(const QByteArray &imageData);  // 캡처 이미지 수신
    void onStatusReceived(const QString &status);               // 상태 메시지 수신

    // 제어 버튼 슬롯들
    void onStartStreamClicked();            // 스트리밍 시작 버튼
    void onStopStreamClicked();             // 스트리밍 정지 버튼
    void onCaptureImageClicked();           // 이미지 캡처 버튼
    void onLCD1Clicked();                   // LCD 1 제어 버튼
    void onLCD2Clicked();                   // LCD 2 제어 버튼
    void onLCD3Clicked();                   // LCD 3 제어 버튼
    void onLCD4Clicked();                   // LCD 4 제어 버튼

    // UI 업데이트 슬롯들
    void updateDateTime();                  // 날짜/시간 업데이트
    void onVideoStreamingStarted();         // 비디오 스트리밍 시작
    void onVideoStreamingStopped();         // 비디오 스트리밍 정지
    void onImageAdded(int totalCount);      // 이미지 추가됨
    
    // 메뉴 액션 슬롯들
    void showAbout();                       // About 다이얼로그 표시
    void toggleFullscreen();                // 전체화면 토글

private:
    // 초기화 함수들
    void setupUIComponents();               // UI 컴포넌트 추가 설정
    void updateConnectionStatus(bool connected);  // 연결 상태 UI 업데이트
    void controlLCD(int lcdId);             // LCD 제어 공통 함수
    void logMessage(const QString &message); // 로그 메시지 출력

    Ui::MainWindow *ui;                     // UI 객체 포인터
    
    // 네트워크 관련 컴포넌트들
    NetworkClient *m_networkClient;         // 네트워크 클라이언트
    VideoStreamer *m_videoStreamer;         // 비디오 스트리머
    ImageViewer *m_imageViewer;             // 이미지 뷰어
    
    // 상태 변수들
    bool m_isConnected;                     // 연결 상태
    bool m_isStreaming;                     // 스트리밍 상태
    QMap<int, bool> m_lcdStates;            // LCD 상태 저장
};

#endif // MAINWINDOW_H
