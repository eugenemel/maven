#!/bin/sh
DATE=`date +%Y%m%d`
SRC=/Users/eugenemelamud/src/mzroll
BUILD=/Users/eugenemelamud/src/mzroll
QTBIN=/Users/eugenemelamud/Qt5.7.0/5.7/clang_64/bin
#QTBIN=~/Qt/5.6/clang_64/bin/
#QTBIN=~/Qt/5.6/clang_64/bin/macdeployqt

#get the latest source
cd $BUILD

#clean up
rm -rf $BUILD/bin/Maven.app $BUILD/bin/Maven.dmg
rm -rf $BUILD/bin/peakdetector.app

#update Makefile
$QTBIN/qmake -r
#make
make clean
make -j10

#copy support files
cd $BUILD/bin
cp $SRC/bin/*.csv   Maven.app/Contents/Resources
cp $SRC/bin/*.model Maven.app/Contents/Resources

mkdir Maven.app/Contents/Resources/methods
mkdir Maven.app/Contents/Resources/pathways
mkdir Maven.app/Contents/Resources/scripts
cp $SRC/bin/methods/* Maven.app/Contents/Resources/methods
cp $SRC/bin/pathways/* Maven.app/Contents/Resources/pathways
cp $SRC/bin/scripts/* Maven.app/Contents/Resources/scripts

#fix Qt dynamic library dependancy
$QTBIN/macdeployqt Maven.app -dmg
$QTBIN/macdeployqt peakdetector.app -dmg
mv Maven.dmg ~/desktop/Maven_$DATE.dmg
