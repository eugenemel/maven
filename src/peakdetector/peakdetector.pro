# codebase paths
CORE = ../maven_core
NATSORT = .
MAVEN = ../maven/

include($$CORE/libmaven.pri)

TEMPLATE = app
TARGET = peakdetector
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

#BUILD OPENMP PARALLEL VERSION

mac {
        exists(/usr/local/opt/llvm) {
            message("mac os x: llvm found. Will build parallel.")

            #compiler and linker need to be clang-omp++
            QMAKE_CXX=/usr/local/opt/llvm/bin/clang++
            QMAKE_LINK=/usr/local/opt/llvm/bin/clang++
            LIBS += -L/usr/local/opt/llvm/lib/

            CONFIG += parallel
        }
}

unix {
    CONFIG += parallel
}

win32 {

}

parallel {
	message("Building OpenMP peakdetector_parallel")
	QMAKE_CXXFLAGS += -DOMP_PARALLEL -fopenmp
        QMAKE_LFLAGS += -fopenmp
	LIBS += -fopenmp
}

SOURCES= peakdetector.cpp  options.cpp $$MAVEN/classifier.cpp $$MAVEN/classifierNeuralNet.cpp $$MAVEN/projectDB.cpp $$MAVEN/database.cpp $$NATSORT/strnatcmp.c
HEADERS= $$MAVEN/classifierNeuralNet.h  options.h $$MAVEN/projectDB.h $$MAVEN/database.h $$NATSORT/strnatcmp.h
