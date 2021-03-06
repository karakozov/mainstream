#-------------------------------------------------
#
# Project created by QtCreator 2014-05-19T11:26:42
#
#-------------------------------------------------

#CONFIG += release
#CONFIG += debug

QT       += core gui widgets

TARGET = gui
TEMPLATE = app

DEFINES += USE_GUI

win32 {
DEFINES += __IPC_WIN__
}

linux-g++ {
DEFINES += __IPC_LIN__
}

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
    ../sysconfig.cpp \
    ../pcie_test.cpp \
    adc_test_thread.cpp \
    pcie_test_thread.cpp

win32 {
    SOURCES += ../gipcy/windows/winipc.cpp
    SOURCES += ../gipcy/windows/devipc.cpp
    SOURCES += ../gipcy/windows/sysipc.cpp
    SOURCES += ../gipcy/windows/fileipc.cpp
}


HEADERS  += mainwindow.h \
    adc_test_thread.h \
    pcie_test_thread.h

FORMS    += mainwindow.ui

linux-g++ {
LIBS += ../gipcy/lib/libgipcy.so
}

