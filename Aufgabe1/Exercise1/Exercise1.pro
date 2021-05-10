# ----------------------------------------------------
# This file is generated by the Qt Visual Studio Add-in.
# ------------------------------------------------------

TEMPLATE = app
TARGET = Exercise1
QT += core opengl widgets gui
CONFIG += debug
DEFINES += QT_DLL QT_OPENGL_LIB QT_WIDGETS_LIB
INCLUDEPATH += ./GeneratedFiles \
    . \
    ./GeneratedFiles/Debug \
    ./external/eigen-3.3.9
LIBS += -lopengl32 -lglu32
CONFIG += c++11
RESOURCES += resources.qrc
DEPENDPATH += .
MOC_DIR += ./GeneratedFiles/debug
OBJECTS_DIR += debug
UI_DIR += ./GeneratedFiles
RCC_DIR += ./GeneratedFiles

HEADERS += ./glwidget.h \
    ./mainwindow.h \
    ./camera.h\
    pointcloud.h \
    pointcloud.h
SOURCES += ./glwidget.cpp \
     ./mainwindow.cpp \
    ./camera.cpp \
    ./main.cpp \
    pointcloud.cpp

FORMS += ./mainwindow.ui
