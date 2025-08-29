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

class CustomTitleBar : public QWidget {
    Q_OBJECT

public:
    explicit CustomTitleBar(QWidget *parent = nullptr);
    ~CustomTitleBar();
    void setTitle(const QString &title);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

signals:
    void minimizeClicked();
    void closeClicked();

private slots:
    void onToolTipClicked();

private:
    QHBoxLayout *titleLayout;
    QLabel *titleLabel;
    QPushButton *minimizeButton;
    QPushButton *closeButton;
    QPushButton *toolTipButton;
    QPoint mousePressPosition;

    QTemporaryFile *m_tempFile = nullptr;

    void setupUi();
};

#endif // CUSTOMTITLEBAR_H
