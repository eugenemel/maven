#!/bin/bash

brew update
brew install qt5
brew link qt5 --force

ENVFILE=qt-5.env
echo Create $ENVFILE
cat << EOF > $ENVFILE
export PATH="/usr/local/opt/qt/bin:$PATH"
export LDFLAGS="-L/usr/local/opt/qt/lib"
export CPPFLAGS="-I/usr/local/opt/qt/include"
export PKG_CONFIG_PATH="/usr/local/opt/qt/lib/pkgconfig"
EOF
