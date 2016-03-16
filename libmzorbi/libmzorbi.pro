include(../mzroll.pri)
DESTDIR = $$OUTPUT_DIR/lib

TEMPLATE=lib
CONFIG += staticlib
TARGET = mzorbi

LIBS += -L. -lcdfread -lnetcdf


INCLUDEPATH += ../pugixml/src/ ../libcdfread/ ../zlib/

SOURCES=base64.cpp mzMassCalculator.cpp mzSample.cpp  mzUtils.cpp statistics.cpp mzFit.cpp mzAligner.cpp\
       PeakGroup.cpp EIC.cpp Scan.cpp Peak.cpp  \
       Compound.cpp \
       savgol.cpp \
       SavGolSmoother.cpp \
       parallelmassSlicer.cpp \
       PolyAligner.cpp \ 
       Fragment.cpp

HEADERS += base64.h mzFit.h mzMassCalculator.h mzSample.h mzPatterns.h mzUtils.h  statistics.h SavGolSmoother.h PolyAligner.h Fragment.h parallelmassSlicer.h
message($$LIBS)
