#-------------------------------------------------
#
# Project created by QtCreator 2015-04-16T11:14:57
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG   += c++11

TARGET = Trem
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    trem.cpp \
    malha.cpp \
    desenhomalha.cpp

HEADERS  += mainwindow.h \
    trem.h \
    malha.h \
    desenhomalha.h

FORMS    += mainwindow.ui
