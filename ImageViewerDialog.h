#ifndef IMAGEVIEWERDIALOG_H
#define IMAGEVIEWERDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QPixmap>

class CustomTitleBar;

class ImageViewerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ImageViewerDialog(QWidget *parent = nullptr);

    void setImage(const QPixmap &pixmap, const QString &timestamp, const QString &logText);

private:
    void setupUI();

    QLabel *m_imageLabel;
    QLabel *m_timestampLabel;
    QTextEdit *m_logTextEdit;
    QScrollArea *m_scrollArea;

    CustomTitleBar *titleBar;
};

#endif // IMAGEVIEWERDIALOG_H
