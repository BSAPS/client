QT += core widgets network multimedia multimediawidgets

CONFIG += c++17

TARGET = CCTVMonitoring
TEMPLATE = app

# 소스 파일
SOURCES += \
    LoginWindow.cpp \
    main.cpp \
    MainWindow.cpp \
    VideoStreamWidget.cpp \
    TcpCommunicator.cpp \
    ImageViewerDialog.cpp \
    NetworkConfigDialog.cpp \
    LineDrawingDialog.cpp \
    EnvConfig.cpp

# 헤더 파일
HEADERS += \
    LoginWindow.h \
    MainWindow.h \
    VideoStreamWidget.h \
    TcpCommunicator.h \
    ImageViewerDialog.h \
    NetworkConfigDialog.h \
    LineDrawingDialog.h \
    EnvConfig.h

# 리소스 파일
RESOURCES += resources.qrc


# 빌드 디렉토리 설정
DESTDIR = build/bin
OBJECTS_DIR = build/obj
MOC_DIR = build/moc
RCC_DIR = build/rcc
UI_DIR = build/ui

# 컴파일러 플래그
QMAKE_CXXFLAGS += -Wno-unused-parameter

FORMS += \
    LoginWindow.ui
