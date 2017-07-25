TEMPLATE = subdirs
CONFIG += ordered qt thread
SUBDIRS += src/maven_core/libneural \
		   src/maven_core/libcdfread \
		   src/maven_core/pugixml/src \
		   src/maven_core/MSToolkit \
		   src/maven_core/libmaven \
		   src/maven

#exists( MSToolkit)  { SUBDIRS += MSToolkit }
#exists( libmaven ) { SUBDIRS += libmaven }
#exists( peakdetector ) { SUBDIRS += peakdetector }
#exists( maven_peakdetector ) { SUBDIRS += maven_peakdetector }
#exists( maven ) { SUBDIRS += maven }
#exists( mzLibraryBrowser )  { SUBDIRS += mzLibraryBrowser }
