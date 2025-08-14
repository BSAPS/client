#ifndef CUSTOMTITLEBAR_H
#define CUSTOMTITLEBAR_H

#include <QWidget>
#include <QPoint>

class QHBoxLayout;
class QPushButton;
class QLabel;
class QMouseEvent;

class CustomTitleBar : public QWidget {
    Q_OBJECT

public:
    explicit CustomTitleBar(QWidget *parent = nullptr);
    void setTitle(const QString &title);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

signals:
    void minimizeClicked();
    void closeClicked();

private:
    QHBoxLayout *titleLayout;
    QLabel *titleLabel;
    QPushButton *minimizeButton;
    QPushButton *closeButton;
    QPoint mousePressPosition;

    void setupUi();
};

#endif // CUSTOMTITLEBAR_H
