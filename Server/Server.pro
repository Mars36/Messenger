QT -= gui
QT += network sql

CONFIG += c++11 console
CONFIG -= app_bundle

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        main.cpp \
        server.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    server.h \
    qtincludes.h


QMAKE_LFLAGS = -lrt

unix:!macx: LIBS = /usr/local/lib/libp7.a
LIBS = /usr/local/lib/*.a

win32: LIBS += -L$$PWD/p7/lib/ -lp7

INCLUDEPATH += $$PWD/p7/include
DEPENDPATH += $$PWD/p7/include

win32:!win32-g++: PRE_TARGETDEPS += $$PWD/p7/lib/p7.lib
else:win32-g++: PRE_TARGETDEPS += $$PWD/p7/lib/libp7.a

win32: LIBS += -L$$PWD/p7/lib/ -lWS2_32

INCLUDEPATH += $$PWD/p7/include
DEPENDPATH += $$PWD/p7/include

win32:!win32-g++: PRE_TARGETDEPS += $$PWD/p7/lib/WS2_32.lib
else:win32-g++: PRE_TARGETDEPS += $$PWD/p7/lib/libWS2_32.a

win32: LIBS += -L$$PWD/p7/lib/ -lWSock32

INCLUDEPATH += $$PWD/p7/include
DEPENDPATH += $$PWD/p7/include

win32:!win32-g++: PRETARGETDEPS += $$PWD/p7/lib/WSock32.lib
else:win32-g++: PRE_TARGETDEPS += $$PWD/p7/lib/libWSock32.a
_
