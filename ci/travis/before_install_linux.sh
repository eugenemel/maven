#!/bin/sh

echo "Issue 723: Do nothing"

# brew update
# brew install qt5
# brew link qt5 --force
# 
# ENVFILE=qt-5.env
# echo Create $ENVFILE
# cat << EOF > $ENVFILE

# Clone QTCI to assist with scripted install of QT
# echo Cloning qtci repository
# git clone https://github.com/benlau/qtci.git
# export PATH="$PATH:$PWD/qtci/bin:$PWD/qtci/recipes"
#
# echo Downloading Qt
# INSTALLER=qt-opensource-linux-x64-5.8.0.run
# ENVFILE=qt-5.8.0.env
# wget -c https://download.qt.io/archive/qt/5.8/5.8.0/qt-opensource-linux-x64-5.8.0.run
# echo Installing Qt
# QT_CI_PACKAGES="qt.58.gcc_64" extract-qt-installer $PWD/$INSTALLER $PWD/Qt
#
# echo Create $ENVFILE
# cat << EOF > $ENVFILE
# export PATH=$PWD/Qt/5.8/gcc_64/bin:$PATH
# EOF

