#ifndef CUSTOMMESSAGEBOX_H
#define CUSTOMMESSAGEBOX_H

#include <QDialog>
#include <QString>

class QLabel;
class QPushButton;
class QVBoxLayout;
class QMouseEvent;

/**
 * @brief 사용자 지정 메시지 박스 클래스
 * @details 사용자 정의 메시지 박스를 생성하고 표시합니다.
 */
class CustomMessageBox : public QDialog {
    Q_OBJECT

public:
    /**
     * @brief CustomMessageBox 생성자
     * @details 사용자 지정 메시지 박스를 생성합니다.
     * @param parent 부모 위젯
     * @param title 메시지 박스 타이틀
     * @param message 표시할 메시지
     */
    explicit CustomMessageBox(QWidget *parent = nullptr, const QString &title = "", const QString &message = "");

protected:
    /**
     * @brief 마우스 클릭 이벤트 처리
     * @details 창 이동을 위한 드래그 시작 위치를 저장합니다.
     * @param event 마우스 이벤트
     */
    void mousePressEvent(QMouseEvent *event) override;
    /**
     * @brief 마우스 이동 이벤트 처리
     * @details 창을 드래그하여 이동합니다.
     * @param event 마우스 이벤트
     */
    void mouseMoveEvent(QMouseEvent *event) override;
    /**
     * @brief 마우스 릴리즈 이벤트 처리
     * @details 드래그 상태를 해제합니다.
     * @param event 마우스 이벤트
     */
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    /** @brief 메시지 라벨 */
    QLabel *messageLabel;
    /** @brief 확인 버튼 */
    QPushButton *okButton;
    /** @brief 메인 레이아웃 */
    QVBoxLayout *mainLayout;

    /** @brief 드래그 상태 플래그 */
    bool m_dragging;
    /** @brief 드래그 위치 */
    QPoint m_dragPos;
};

#endif // CUSTOMMESSAGEBOX_H
