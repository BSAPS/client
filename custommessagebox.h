#ifndef CUSTOMMESSAGEBOX_H
#define CUSTOMMESSAGEBOX_H

#include <QDialog>
#include <QString>

class QLabel;
class QPushButton;
class QVBoxLayout;
class QMouseEvent;

class CustomMessageBox : public QDialog {
    Q_OBJECT // Q_OBJECT 매크로는 시그널/슬롯 메커니즘을 사용하기 위해 필요합니다.

public:
    explicit CustomMessageBox(QWidget *parent = nullptr, const QString &title = "", const QString &message = "");
    // 소멸자가 필요한 경우 여기에 선언할 수 있습니다.
    // ~CustomMessageBox();

    // 다이얼로그의 너비와 높이를 설정하는 함수 추가
    void setFixedSize(int width, int height);
    void setFixedSize(const QSize &size);

protected:
    // 창 이동을 위한 마우스 이벤트
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QLabel *messageLabel;
    QPushButton *okButton;
    QVBoxLayout *mainLayout;

    bool m_dragging;
    QPoint m_dragPos;
};

#endif // CUSTOMMESSAGEBOX_H
