#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    // 애플리케이션 정보 설정
    a.setApplicationName("CCTV Client");
    a.setApplicationVersion("1.0");
    a.setOrganizationName("CCTV Systems");
    
    MainWindow w;
    w.show();
    
    return a.exec();
}
