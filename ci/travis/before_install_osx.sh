#!/bin/bash

#echo "Maven debugging: try doing nothing for mac os x"

brew update
which cmake
echo $PATH
brew install cmake
brew install qt@5
brew link qt5 --force

ENVFILE=qt-5.env
echo Create $ENVFILE
cat << EOF > $ENVFILE

export PATH="/opt/homebrew/opt/qt@5/bin:$PATH"
export LDFLAGS="-L/opt/homebrew/opt/qt@5/lib"
export CPPFLAGS="-I/opt/homebrew/opt/qt@5/include"
export PKG_CONFIG_PATH="/opt/homebrew/opt/pkgconfig/"
EOF