#ifndef IMAGEVIEWERDIALOG_H
#define IMAGEVIEWERDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QPixmap>

class CustomTitleBar;

/**
 * @brief 이미지 뷰어 다이얼로그 클래스
 * @details 이미지와 로그 정보를 표시하는 다이얼로그입니다.
 */
class ImageViewerDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief ImageViewerDialog 생성자
     * @param parent 부모 위젯
     */
    explicit ImageViewerDialog(QWidget *parent = nullptr);
    /**
     * @brief 이미지 및 로그 정보 설정
     * @param pixmap 이미지
     * @param timestamp 촬영 시간
     * @param logText 로그 텍스트
     */
    void setImage(const QPixmap &pixmap, const QString &timestamp, const QString &logText);

private:
    /** @brief UI 설정 함수 */
    void setupUI();
    /** @brief 이미지 라벨 */
    QLabel *m_imageLabel;
    /** @brief 타임스탬프 라벨 */
    QLabel *m_timestampLabel;
    /** @brief 로그 텍스트 에디터 */
    QTextEdit *m_logTextEdit;
    /** @brief 스크롤 영역 */
    QScrollArea *m_scrollArea;
    /** @brief 커스텀 타이틀바 */
    CustomTitleBar *titleBar;
};

#endif // IMAGEVIEWERDIALOG_H
