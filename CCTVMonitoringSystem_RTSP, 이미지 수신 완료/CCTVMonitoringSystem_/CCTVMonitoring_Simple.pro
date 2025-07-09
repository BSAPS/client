QT += core widgets network

CONFIG += c++17

TARGET = CCTVMonitoring
TEMPLATE = app

# 먼저 OpenCV 없이 테스트
# INCLUDEPATH += C:/opencv/build/include
# LIBS += -LC:/opencv/build/x64/vc16/lib -lopencv_world4100d

SOURCES += \
    main_simple.cpp \
    MainWindow_simple.cpp

HEADERS += \
    MainWindow_simple.h

DESTDIR = build/bin
OBJECTS_DIR = build/obj
MOC_DIR = build/moc
