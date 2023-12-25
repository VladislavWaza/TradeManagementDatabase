QT       += core gui
QT += sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    authorization.cpp \
    main.cpp \
    mainwindow.cpp \
    selectdialog.cpp \
    shopreport.cpp \
    showingform.cpp \
    trade_management_db.cpp

HEADERS += \
    authorization.h \
    mainwindow.h \
    selectdialog.h \
    shopreport.h \
    showingform.h \
    trade_management_db.h

FORMS += \
    authorization.ui \
    mainwindow.ui \
    selectdialog.ui \
    shopreport.ui \
    showingform.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
