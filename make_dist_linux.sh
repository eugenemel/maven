#!/bin/bash
set -e

#Set Script Name variable
SCRIPT=`basename ${BASH_SOURCE[0]}`

#Set fonts for Help.
NORM=`tput sgr0`
BOLD=`tput bold`
REV=`tput smso`

#Help function
function HELP {
    echo -e \\n"${REV}Basic usage:${NORM} ${BOLD}$SCRIPT file.exe${NORM}"\\n
    echo -e "${REV}-h${NORM}  --Displays this help message. No further functions are performed."\\n
    echo -e "Example: ${BOLD}$SCRIPT \"bin/maven_dev_20170405\"${NORM}"\\n
    exit 1
}

#Check the number of arguments. If none are passed, print help and exit.
NUMARGS=$#
if [ $NUMARGS -eq 0 ]; then
    HELP
fi

# A POSIX variable
OPTIND=1         # Reset in case getopts has been used previously in the shell.

# Initialize our own variables:

while getopts "h?f:" opt; do
    case "$opt" in
    h|\?)
        show_help
        exit 0
        ;;
    esac
done

shift $((OPTIND-1))  #This tells getopts to move on to the next argument.

binpath=$1
bindir=${binpath%/*}
binfn="${binpath##*/}"
distpath="dist"

# TODO Resources must be copied to appdir prior to packaging
# See: RedTimer example at https://github.com/probonopd/linuxdeployqt/wiki
#echo "Copying resources to ${apppath}"
#mkdir -p "${apppath}/Contents/Resources/methods"
#cp src/maven_core/bin/methods/* "${apppath}/Contents/Resources/methods"
# TODO Should pathways be in the repo?
# mkdir "${apppath}/Contents/Resources/pathways"
# cp bin/pathways/* "${apppath}/Contents/Resources/pathways"
#mkdir -p "${apppath}/Contents/Resources/scripts"
#cp src/maven_core/bin/scripts/* "${apppath}/Contents/Resources/scripts"

# Download linuxdeployqt
echo "Downloading linuxdeployqt"
wget -c "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"
chmod a+x linuxdeployqt*.AppImage
unset QTDIR; unset QT_PLUGIN_PATH; unset LD_LIBRARY_PATH

echo "Running linuxdeployqt"
./linuxdeployqt*.AppImage "${binpath}" -bundle-non-qt-libs -verbose=2
./linuxdeployqt*.AppImage "${binpath}" -appimage -verbose=2
find "${bindir}" -executable -type f -exec ldd {} \; | grep " => /usr" | cut -d " " -f 2-3 | sort | uniq
#- curl --upload-file ./APPNAME*.AppImage https://transfer.sh/APPNAME-git.$(git rev-parse --short HEAD)-x86_64.AppImage
ls -la "${binpath}"

echo "Copying AppImage to ${distpath}"
mkdir -p "${distpath}"
ls -la
cp -v "Maven-${VERSION}-x86_64.AppImage" "${distpath}/"
ls -la "${distpath}"

