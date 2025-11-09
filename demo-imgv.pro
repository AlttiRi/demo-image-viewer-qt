QT     += core gui widgets
QT     += concurrent
CONFIG += c++17
# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
VERSION = 0.1.4

QT_MIN_VERSION = 6.6.0
!versionAtLeast(QT_VERSION, $$QT_MIN_VERSION) {
    error("Qt version must be at least "$$QT_MIN_VERSION". Current version is "$$QT_VERSION".")
} else {
    # message("Qt version is" $$QT_VERSION".")
}

SOURCES += \
    win.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    core.h \
    win.h \
    mainwindow.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
