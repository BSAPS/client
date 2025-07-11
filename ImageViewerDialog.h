#ifndef IMAGEVIEWERDIALOG_H
#define IMAGEVIEWERDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QPixmap>

class ImageViewerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ImageViewerDialog(QWidget *parent = nullptr);
    ~ImageViewerDialog();

    void setImage(const QPixmap &pixmap, const QString &timestamp, const QString &logText);

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    void setupUI();

    QLabel *m_imageLabel;
    QLabel *m_timestampLabel;
    QTextEdit *m_logTextEdit;
    QPushButton *m_closeButton;
    QScrollArea *m_scrollArea;
};

#endif // IMAGEVIEWERDIALOG_H
