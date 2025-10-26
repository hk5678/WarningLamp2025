QT       += core gui
QT       += charts  sql serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

QMAKE_LFLAGS += -static

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    TempSensor.cpp \
    historywin.cpp \
    main.cpp \
    qmserial.cpp \
    warninglamp.cpp \

HEADERS += \
    TempSensor.h \
    historywin.h \
    qmserial.h \
    warninglamp.h

FORMS += \
    historywin.ui \
    warninglamp.ui

TRANSLATIONS += \
    WarningLamp2025_zh_CN.ts
CONFIG += lrelease
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    ../build-WarningLamp2025-Desktop_Qt_6_3_2_MinGW_64_bit-Debug/debug/config.ini \
    build/Desktop_Qt_6_5_3_MinGW_64_bit-Release/release/config.ini \
    build/Desktop_Qt_6_5_3_MinGW_64_bit-Release/release/config.ini \
    config.ini \
    images/clean.png \
    images/lamp.png \
    images/start.png \
    images/start_whiite.png \
    warning.ico \
    warning.log

RC_FILE = demo.rc
