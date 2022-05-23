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

#TO ACTIVE BUILD RUN: qmake CONFIG+=parallel
CONFIG += parallel

mac { 	#compiler and linker need to be clang-omp++
        QMAKE_CXX=/usr/local/Cellar/llvm/13.0.1_1/bin/clang++
        QMAKE_LINK=/usr/local/Cellar/llvm/13.0.1_1/bin/clang++

        #Phil: these includes confuse my Qt Creator
        #QMAKE_CXXFLAGS += -I/usr/local//Cellar/libomp/include/
        #QMAKE_CXXFLAGS += -I/usr/local/Cellar//llvm/9.0.0_1/include/c++/v1

        QMAKE_CXXFLAGS += -I/usr/local/Cellar/llvm/9.0.0_1/lib/clang/9.0.0_1/include/
        QMAKE_CXXFLAGS += -I/usr/local/Cellar/llvm/13.0.1_1/lib/clang/13.0.1/include/

		QMAKE_CXXFLAGS += -I/usr/lib
		QMAKE_CXXFLAGS += -I/
		
        LIBS += -L/usr/local/Cellar/libomp/9.0.0_1/lib/
        LIBS += -L/usr/local/opt/llvm/lib/
}


parallel {
	message("Building OpenMP peakdetector_parallel")
	QMAKE_CXXFLAGS += -DOMP_PARALLEL -fopenmp
        QMAKE_LFLAGS += -fopenmp
	LIBS += -fopenmp
}

SOURCES= peakdetector.cpp  options.cpp $$MAVEN/classifier.cpp $$MAVEN/classifierNeuralNet.cpp $$MAVEN/projectDB.cpp $$MAVEN/database.cpp $$NATSORT/strnatcmp.c
HEADERS= $$MAVEN/classifierNeuralNet.h  options.h $$MAVEN/projectDB.h $$MAVEN/database.h $$NATSORT/strnatcmp.h