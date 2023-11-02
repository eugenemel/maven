#!/bin/bash

#echo "Maven debugging: try doing nothing for mac os x"

brew update
brew install qt5
brew link qt5 --force

ENVFILE=qt-5.env
echo Create $ENVFILE
cat << EOF > $ENVFILE

# OLD as of issue #681
# export PATH="/usr/local/opt/qt/bin:$PATH"
# export LDFLAGS="-L/usr/local/opt/qt/lib"
# export CPPFLAGS="-I/usr/local/opt/qt/include"
# export PKG_CONFIG_PATH="/usr/local/opt/qt/lib/pkgconfig"

export PATH="/opt/homebrew/opt/qt@5/bin:$PATH"
export LDFLAGS="-L/opt/homebrew/opt/qt@5/lib"
export CPPFLAGS="-I/opt/homebrew/opt/qt@5/include"
export PKG_CONFIG_PATH="/opt/homebrew/opt/pkgconfig/"
EOF