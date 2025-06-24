#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <QWidget>              // 위젯 기본 클래스
#include <QLabel>               // 라벨 위젯
#include <QScrollArea>          // 스크롤 영역
#include <QVBoxLayout>          // 수직 레이아웃
#include <QHBoxLayout>          // 수평 레이아웃
#include <QPushButton>          // 푸시 버튼
#include <QSlider>              // 슬라이더
#include <QPixmap>              // 픽스맵
#include <QList>                // 리스트 컨테이너

// 이미지 뷰어 위젯 클래스
class ImageViewer : public QWidget
{
    Q_OBJECT

public:
    explicit ImageViewer(QWidget *parent = nullptr);     // 생성자
    ~ImageViewer();                                      // 소멸자

    // 이미지 관리
    void addImage(const QByteArray &imageData);          // 이미지 추가
    void addImage(const QPixmap &pixmap);                // 픽스맵 추가
    void clearImages();                                  // 모든 이미지 클리어
    void saveCurrentImage(const QString &fileName);      // 현재 이미지 저장

    // 이미지 네비게이션
    void showImage(int index);                           // 특정 인덱스 이미지 표시
    void showNextImage();                                // 다음 이미지 표시
    void showPreviousImage();                            // 이전 이미지 표시
    void showFirstImage();                               // 첫 번째 이미지 표시
    void showLastImage();                                // 마지막 이미지 표시

    // 확대/축소
    void zoomIn();                                       // 확대
    void zoomOut();                                      // 축소
    void resetZoom();                                    // 원본 크기
    void fitToWindow();                                  // 창 크기에 맞춤

    // 상태 조회
    int getImageCount() const;                           // 이미지 개수
    int getCurrentIndex() const;                         // 현재 인덱스
    double getZoomFactor() const;                        // 현재 확대 비율

signals:
    void imageChanged(int index);                        // 이미지 변경 시그널
    void imageAdded(int totalCount);                     // 이미지 추가 시그널
    void zoomChanged(double factor);                     // 확대 비율 변경 시그널

private slots:
    void onPreviousClicked();                            // 이전 버튼 클릭
    void onNextClicked();                                // 다음 버튼 클릭
    void onZoomInClicked();                              // 확대 버튼 클릭
    void onZoomOutClicked();                             // 축소 버튼 클릭
    void onResetZoomClicked();                           // 원본 크기 버튼 클릭
    void onFitToWindowClicked();                         // 창 맞춤 버튼 클릭
    void onSaveClicked();                                // 저장 버튼 클릭
    void onZoomSliderChanged(int value);                 // 확대 슬라이더 변경

private:
    // UI 설정
    void setupUI();                                      // UI 구성
    void updateUI();                                     // UI 업데이트
    void updateImageDisplay();                           // 이미지 표시 업데이트

    // 내부 함수들
    QPixmap scalePixmap(const QPixmap &pixmap, double factor); // 픽스맵 스케일링
    void setZoomFactor(double factor);                   // 확대 비율 설정

    // UI 컴포넌트들
    QVBoxLayout *m_mainLayout;                           // 메인 레이아웃
    QHBoxLayout *m_controlLayout;                        // 컨트롤 레이아웃
    QScrollArea *m_scrollArea;                           // 스크롤 영역
    QLabel *m_imageLabel;                                // 이미지 표시 라벨
    
    // 컨트롤 버튼들
    QPushButton *m_previousButton;                       // 이전 버튼
    QPushButton *m_nextButton;                           // 다음 버튼
    QPushButton *m_zoomInButton;                         // 확대 버튼
    QPushButton *m_zoomOutButton;                        // 축소 버튼
    QPushButton *m_resetZoomButton;                      // 원본 크기 버튼
    QPushButton *m_fitToWindowButton;                    // 창 맞춤 버튼
    QPushButton *m_saveButton;                           // 저장 버튼
    QSlider *m_zoomSlider;                               // 확대 슬라이더
    QLabel *m_infoLabel;                                 // 정보 라벨
    
    // 데이터 멤버들
    QList<QPixmap> m_images;                             // 이미지 리스트
    int m_currentIndex;                                  // 현재 이미지 인덱스
    double m_zoomFactor;                                 // 현재 확대 비율
    QPixmap m_originalPixmap;                            // 원본 픽스맵
};

#endif // IMAGEVIEWER_H
