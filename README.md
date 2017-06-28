
# Maven GUI: Metabolomics Analysis and Visualization Engine

Appveyor: [![Appveyor Build Status](https://ci.appveyor.com/api/projects/status/github/eugenemel/maven?branch=master&svg=true&retina=true)](https://ci.appveyor.com/project/eugenemel/maven)
Travis: [![Travis Build Status](https://travis-ci.org/eugenemel/maven.svg?branch=master)](https://travis-ci.org/eugenemel/maven)


## Compile

    git clone --recursive  git@github.com:eugenemel/maven.git maven
    cd maven
    qmake -r
    make -j4


## Make distributable package

    make INSTALL_ROOT=appdir install
    make_dist_[platform].sh src/maven/appdir/bin/[executable]


*platform* is one of `win32`, `osx`, or `linux`

*executable* is the name of the executable (e.g. `maven_dev_800.exe` or
    `Maven.app`)


## Dependencies

### Windows

1.  Install the [MSYS2 platform for Windows](http://www.msys2.org/)

2.  From a mingw64 prompt, install 64-bit QT, zlib, and sqlite

        pacman -S --needed --noconfirm git
        pacman -S --needed --noconfirm make
        pacman -S --needed --noconfirm mingw-w64-x86_64-qt-creator
        pacman -S --needed --noconfirm zlib-devel
        pacman -S --needed --noconfirm mingw64/mingw-w64-x86_64-sqlite3

*Note*: There is a [bug in MSYS2 packages](https://github.com/Alexpux/MSYS2-packages/issues/735) that causes the following error:

    Cloning into 'maven_core'...
    warning: redirecting to https://github.com/eugenemel/maven_core/
    remote: Counting objects: 694, done.
    remote: Total 694 (delta 0), reused 0 (delta 0), pack-reused 694
    Receiving objects: 100% (694/694), 1.53 MiB | 0 bytes/s, done.
    Resolving deltas: 100% (153/153), done.
    Checking out files: 100% (576/576), done.
    ': not a valid identifierline 88: export: `dashless

[WORKAROUND](https://github.com/Alexpux/MSYS2-packages/issues/735#issuecomment-264556713)

    mv /mingw64/bin/envsubst.exe /mingw64/bin/envsubst.exe.orig
    pacman -S gettext


### OSX / MacOS

1.  Install the [Homebrew package management system]()

2.  Install the qt5 package

        brew update
        brew install qt5

3.  Setup the environment for the newly installed qt5

        export PATH="/usr/local/opt/qt/bin:$PATH"
        export LDFLAGS="-L/usr/local/opt/qt/lib"
        export CPPFLAGS="-I/usr/local/opt/qt/include"
        export PKG_CONFIG_PATH="/usr/local/opt/qt/lib/pkgconfig"


### Linux (Ubuntu 16.04 LTS, Xenial)

1.  Add the [PPA to install QT 5.8](https://launchpad.net/~beineri/+archive/ubuntu/opt-qt58-xenial)

        sudo add-apt-repository ppa:beineri/opt-qt58-xenial
        sudo apt-get update

2.  Install QT, opengl libraries, sqlite3, mysql, and libssl-dev

        sudo apt-get install qt58-meta-minimal \
                             mesa-common-dev \
                             libgl1-mesa-dev \
                             libsqlite3-dev \
                             libmysqlclient-dev \
                             libssl1.0.0 \
                             libssl-dev

