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

INSTALLS += target
linux:INSTALLS += desktop
target.path =  $${INSTALL_PREFIX}/bin

QT += sql gui
QT -= network opengl std++14

INCLUDEPATH += $$CORE/pugixml/src/ $$CORE/libmaven $$CORE/pugixml/src $$CORE/libneural $$CORE/MSToolkit/include $$CORE $$NATSORT

LDFLAGS  +=  $$OUTPUT_DIR/lib

LIBS += -L. -L$$CORE/build/lib  -lmaven -lpugixml -lneural -lmstoolkitlite -lz

contains (DEFINES,CDFPARSER) {
    LIBS +=  -lcdfread -lnetcdf
}

#add version information during compilation
VERSION = $$system("git describe --tags --always")

DEFINES += MAVEN_VERSION=\\\"$$VERSION\\\"
DEFINES += "PLATFORM=\"$$QMAKE_HOST.os\""

message("Peakdetector Version is:")
message($$VERSION)

#BUILD OPENMP PARALLEL VERSION

mac {
        exists(/usr/local/opt/llvm) {
            message("mac os x: llvm found. Will build parallel.")

            #compiler and linker need to be clang-omp++
            QMAKE_CXX=/usr/local/opt/llvm/bin/clang++
            QMAKE_LINK=/usr/local/opt/llvm/bin/clang++
            LIBS += -L/usr/local/opt/llvm/lib/

	    INCLUDEPATH += /usr/local/opt/llvm/include/
            INCLUDEPATH += /usr/local/opt/llvm/lib/include/

            # Change this as needed for version of llvm
            # INCLUDEPATH += /usr/local/opt/llvm/lib/clang/16/include/

            CONFIG += parallel
        }
}

unix {
    !mac {
            CONFIG += parallel
    }
}

win32 {
	CONFIG -= parallel
}

parallel {
	message("Building OpenMP peakdetector_parallel")

        QMAKE_CXXFLAGS += -DOMP_PARALLEL -fopenmp -I/usr/local/opt/libomp/include/
        QMAKE_LFLAGS += -fopenmp
        LIBS += -fopenmp
}

SOURCES= peakdetector.cpp  options.cpp $$MAVEN/classifier.cpp $$MAVEN/classifierNeuralNet.cpp $$MAVEN/projectDB.cpp $$MAVEN/database.cpp $$NATSORT/strnatcmp.c
HEADERS= $$MAVEN/classifierNeuralNet.h  options.h $$MAVEN/projectDB.h $$MAVEN/database.h $$NATSORT/strnatcmp.h
