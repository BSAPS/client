#ifndef CUSTOMTITLEBAR_H
#define CUSTOMTITLEBAR_H

#include <QWidget>
#include <QPoint>
#include <QString>
#include <QFile>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QDebug>
#include <QTemporaryFile>
#include <QMessageBox>

class QHBoxLayout;
class QPushButton;
class QLabel;
class QMouseEvent;

/**
 * @brief 커스텀 타이틀바 클래스
 * @details 프레임리스 윈도우용 커스텀 타이틀바를 구현합니다.
 */
class CustomTitleBar : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief CustomTitleBar 생성자
     * @param parent 부모 위젯
     */
    explicit CustomTitleBar(QWidget *parent = nullptr);
    /**
     * @brief CustomTitleBar 소멸자
     */
    ~CustomTitleBar();
    /**
     * @brief 타이틀 텍스트 설정
     * @param title 타이틀 문자열
     */
    void setTitle(const QString &title);

protected:
    /**
     * @brief 마우스 클릭 이벤트 처리
     * @param event 마우스 이벤트
     */
    void mousePressEvent(QMouseEvent *event) override;
    /**
     * @brief 마우스 이동 이벤트 처리
     * @param event 마우스 이벤트
     */
    void mouseMoveEvent(QMouseEvent *event) override;

signals:
    /** @brief 최소화 버튼 클릭 시그널 */
    void minimizeClicked();
    /** @brief 닫기 버튼 클릭 시그널 */
    void closeClicked();

private slots:
    /** @brief 툴팁 버튼 클릭 슬롯 */
    void onToolTipClicked();

private:
    /** @brief 타이틀 레이아웃 */
    QHBoxLayout *titleLayout;
    /** @brief 타이틀 라벨 */
    QLabel *titleLabel;
    /** @brief 최소화 버튼 */
    QPushButton *minimizeButton;
    /** @brief 닫기 버튼 */
    QPushButton *closeButton;
    /** @brief 툴팁 버튼 */
    QPushButton *toolTipButton;
    /** @brief 마우스 클릭 위치 */
    QPoint mousePressPosition;
    /** @brief 임시 파일 포인터 */
    QTemporaryFile *m_tempFile = nullptr;
    /** @brief UI 설정 함수 */
    void setupUi();
};

#endif // CUSTOMTITLEBAR_H
