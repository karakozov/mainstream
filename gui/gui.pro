#-------------------------------------------------
#
# Project created by QtCreator 2014-05-19T11:26:42
#
#-------------------------------------------------

CONFIG += debug

QT       += core gui

TARGET = gui
TEMPLATE = app

DEFINES += USE_GUI

INCLUDEPATH += .. ../gipcy/include


SOURCES += main.cpp\
        mainwindow.cpp \
    ../i2c.cpp \
    ../fpga_base.cpp \
    ../fpga.cpp \
    ../exceptinfo.cpp \
    ../acsync.cpp \
    ../acdsp.cpp \
    ../trd_check.cpp \
    ../stream.cpp \
    ../si571.cpp \
    ../pe_chn_tx.cpp \
    ../pe_chn_rx.cpp \
    ../memory.cpp \
    ../mapper.cpp \
    ../isvi.cpp \
    ../iniparser.cpp \
    ../../mainstream/sysconfig.cpp

HEADERS  += mainwindow.h

FORMS    += mainwindow.ui

LIBS += ../gipcy/lib/libgipcy.so
