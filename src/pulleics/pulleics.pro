# codebase paths
CORE = ../maven_core
NATSORT = .
MAVEN = ../maven/

include($$CORE/libmaven.pri)

TEMPLATE = app
TARGET = pulleics
DESTDIR = $$MAVEN/bin/
CONFIG -= network gui opengl
CONFIG += sql qt std++14

QT += sql gui
QT -= network opengl std++14

INCLUDEPATH += $$CORE/pugixml/src/ $$CORE/libmaven $$CORE/pugixml/src $$CORE/libneural $$CORE/MSToolkit/include $$CORE $$NATSORT

LDFLAGS  +=  $$OUTPUT_DIR/lib

LIBS += -L. -L$$CORE/build/lib  -lmaven -lpugixml -lneural -lmstoolkitlite -lz

contains (DEFINES,CDFPARSER) {
    LIBS +=  -lcdfread -lnetcdf
}

SOURCES= pulleics.cpp