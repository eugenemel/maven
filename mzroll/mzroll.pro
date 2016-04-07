include(../mzroll.pri)
#TEMPLATE = app
CONFIG += qt thread sql svg console std++14
QMAKE_STRIP=echo
#PRECOMPILED_HEADER  = stable.h

#add version information during compilation
VERSION="20160118"
DEFINES += "MAVEN_VERSION=$$VERSION"
DEFINES += "PLATFORM=\"$$QMAKE_HOST.os\""

TARGET = maven_dev_$$VERSION
macx:TARGET=Maven

RC_FILE = mzroll.rc
RESOURCES +=  mzroll.qrc
ICON = images/icon.icns

DESTDIR = ../bin

QT += sql xml printsupport opengl
QT += network


#DEFINES += QT_CORE_LIB QT_DLL QT_NETWORK_LIB QT_SQL_LIB QT_THREAD_LIB
#INCLUDEPATH +=  /usr/include/qt4/QtXml/ /usr/include/qt/QtSql

INCLUDEPATH += ../libmzorbi ../mzorbi ../pugixml/src ../libneural
LIBS += -L. -L../lib -L../build/lib -lmzorbi -lpugixml -lneural -lz
message($$LIBS)

INSTALLS += sources target

FORMS = forms/settingsform.ui  \
		forms/masscalcwidget.ui \
		forms/isotopeswidget.ui \
		forms/peakdetectiondialog.ui \
        forms/comparesamplesdialog.ui \
        forms/trainingdialog.ui \
        forms/alignmentdialog.ui \
        forms/rconsolewidget.ui \
        forms/clusterdialog.ui \
        forms/spectramatching.ui \
    forms/librarydialog.ui

HEADERS +=  stable.h \
                    globals.h \
                    mainwindow.h \
                    tinyplot.h \
                    plotdock.h \
                    settingsform.h \
                    database.h \
                    classifier.h \
                    classifierNeuralNet.h \
                    csvreports.h \
                    background_peaks_update.h \
                    isotopeplot.h\
                    barplot.h \
                    boxplot.h \
                    line.h \
                    point.h \
                    history.h \
                    plot_axes.h \
                    spectrawidget.h\
                    masscalcgui.h \
                    isotopeswidget.h \
                    ligandwidget.h \
                    eicwidget.h \
                    peakdetectiondialog.h \
                    comparesamplesdialog.h \
                    traindialog.h \
                    tabledockwidget.h  \
                    treedockwidget.h  \
                    heatmap.h  \
                    note.h  \
                    suggest.h \
                    alignmentdialog.h \ 
                    scatterplot.h \
                    gallerywidget.h \
                    rconsolewidget.h \
                    highlighter.h \
                    projectdockwidget.h \
                    spectramatching.h \
                    mzfileio.h \
                    spectralhit.h \
                    clusterdialog.h \
                    projectDB.h \
                    librarydialog.h


SOURCES += mainwindow.cpp  \
classifier.cpp \
classifierNeuralNet.cpp \
database.cpp \
csvreports.cpp \
 plotdock.cpp \
 treedockwidget.cpp \
 tinyplot.cpp \
 settingsform.cpp \
 background_peaks_update.cpp \
 isotopeplot.cpp \
 barplot.cpp \
 boxplot.cpp \
 point.cpp \
 history.cpp \
 spectrawidget.cpp \
 masscalcgui.cpp \
 isotopeswidget.cpp \
 ligandwidget.cpp \
 main.cpp \
 eicwidget.cpp \
 plot_axes.cpp \
 tabledockwidget.cpp \
 peakdetectiondialog.cpp \
 comparesamplesdialog.cpp \
 traindialog.cpp \
 line.cpp  \
 heatmap.cpp \
 note.cpp  \
 suggest.cpp \
 alignmentdialog.cpp\
 scatterplot.cpp \
 gallerywidget.cpp \
 highlighter.cpp \
 rconsolewidget.cpp \
 clusterdialog.cpp \
 projectdockwidget.cpp \
 spectramatching.cpp \
 mzfileio.cpp \
 projectDB.cpp \
 librarydialog.cpp


sources.files =  $$HEADERS \
  $$RESOURCES \
  $$SOURCES \
  *.doc \
  *.html \
  *.pro \
  images
 sources.path =  ./src
 target.path =  ../bin

build_all {
  !build_pass {
   CONFIG -=    build_all
  }
}


