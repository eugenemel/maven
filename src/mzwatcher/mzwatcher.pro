# codebase paths
CORE = ../maven_core
MAVEN = ../maven/

include($$CORE/libmaven.pri)

TEMPLATE = app
FORMS = mzWatcherGui.ui

TARGET = mzwatcher
DESTDIR = $$MAVEN/bin/

INSTALLS += target
target.path = $${INSTALL_PREFIX}/bin

CONFIG += warn_off qt
QT += sql network widgets

DEFINES += QT_CORE_LIB QT_DLL QT_NETWORK_LIB QT_SQL_LIB QT_NO_DEBUG QT_THREAD_LIB

RESOURCES += mzwatcher.qrc

SOURCES = mzWatcher.cpp mainWindow.cpp
HEADERS = mainWindow.h

RC_FILE = mzWatcher.rc
