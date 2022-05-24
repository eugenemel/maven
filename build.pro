TEMPLATE = subdirs
CONFIG += ordered qt thread
SUBDIRS += src/maven_core/libneural \
		   src/maven_core/libcdfread \
		   src/maven_core/pugixml/src \
		   src/maven_core/MSToolkit \
		   src/maven_core/libmaven \
		   src/maven \
		   src/maven_core/mzDeltas \
		   src/peakdetector