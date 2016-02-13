include(../mzroll.pri)

TEMPLATE = app
TARGET = peakdetector
DESTDIR = ../bin
CONFIG -= network gui opengl
CONFIG += warn_off xml sql qt std++14

QT += sql
QT -= network gui opengl std++14

INCLUDEPATH += ../pugixml/src/ ../sqlite ../libmzorbi ../pugixml/src ../libneural ../zlib/ ../libharu/include

LDFLAGS  +=  $$OUTPUT_DIR/lib 

LIBS += -L. -L../build/lib  -lmzorbi -lpugixml -lneural -lz

contains (DEFINES,CDFPARSER) {
    LIBS +=  -lcdfread -lnetcdf
}


#BUILD OPENMP PARALLEL VERSION
#TO ACTIVE BUILD RUN: qmake CONFIG+=parallel 
parallel {
	message("Building OpenMP peakdetector_parallel")
	TARGET = peakdetector_parallel
	QMAKE_CXXFLAGS += -DOMP_PARALLEL -fopenmp
	LIBS += -fopenmp
}



SOURCES= peakdetector.cpp  options.cpp ../mzroll/classifier.cpp ../mzroll/classifierNeuralNet.cpp parallelMassSlicer.cpp ../mzroll/projectDB.cpp ../mzroll/database.cpp
HEADERS= ../mzroll/classifier.h  ../mzroll/classifierNeuralNet.h  options.h  parallelMassSlicer.h ../mzroll/projectDB.h ../mzroll/database.h
