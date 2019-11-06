include(../maven_core/libmaven.pri)
TEMPLATE = app
DESTDIR = bin
CONFIG += qt thread sql svg console std++14
CONFIG+=sdk_no_version_check
QMAKE_STRIP=echo
#PRECOMPILED_HEADER  = stable.h


target.path =  $${INSTALL_PREFIX}/bin
desktop.path = $${INSTALL_PREFIX}/share/applications
desktop.files = maven.desktop

#add version information during compilation
VERSION = $$system("git describe --tags --always")

#TODO: this call is setting VERSION back to an empty string
#include(gitversion.pri)

DEFINES += MAVEN_VERSION=\\\"$$VERSION\\\"
DEFINES += "PLATFORM=\"$$QMAKE_HOST.os\""

message("Maven Version is:")
message($$VERSION)

TARGET = Maven

RC_FILE = mzroll.rc
RESOURCES +=  mzroll.qrc
ICON = images/icon.icns

INSTALLS += target
linux:INSTALLS += desktop

QT += sql xml printsupport opengl network

INCLUDEPATH += ../maven_core/libmaven src ../maven_core/pugixml/src ../maven_core/libneural ../maven_core/MSToolkit/include
LIBS += -L. -L../maven_core/lib -L../maven_core/build/lib -L../maven_core/MSToolkit -lmaven -lpugixml -lneural -lmstoolkitlite -lz
message($$LIBS)

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
        forms/calibrateform.ui \
        forms/librarydialog.ui \
        forms/directinfusiondialog.ui \
        forms/editpeakgroupdialog.ui


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
            librarydialog.h \
            calibratedialog.h \
            directinfusiondialog.h \
            background_directinfusion_update.h \
            editpeakgroupingdialog.h

SOURCES +=  mainwindow.cpp  \
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
            librarydialog.cpp \
            calibratedialog.cpp \
            directinfusiondialog.cpp \
            background_directinfusion_update.cpp \
            editpeakgroupingdialog.cpp


sources.files =  $$HEADERS \
  $$RESOURCES \
  $$SOURCES \
  *.doc \
  *.html \
  *.pro \
  images
sources.path =  .

build_all {
  !build_pass {
   CONFIG -=    build_all
  }
}


