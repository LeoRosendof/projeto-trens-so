QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG   += c++11 thread

QMAKE_CXXFLAGS += -pthread
LIBS     += -pthread

TARGET = Trem
TEMPLATE = app

SOURCES += main.cpp \
    mainwindow.cpp \
    trem.cpp \
    malha.cpp \
    desenhomalha.cpp

HEADERS  += mainwindow.h \
    trem.h \
    malha.h \
    desenhomalha.h

FORMS    += mainwindow.ui
