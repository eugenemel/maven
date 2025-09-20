#!/bin/bash
set -e

echo "Starting maven_dist_win32.sh."

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
    echo -e "Example: ${BOLD}$SCRIPT \"appdir/bin/Maven.exe\"${NORM}"\\n
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

echo "Attempting to determine version..."
GIT_VERSION=$(src/maven/get_version.sh)
echo "Version: "${GIT_VERSION}

exepath=$1
exefn="${exepath##*/}"
distpath="dist/${exefn%.exe}-Windows"
zipfn="${exefn%.exe}_${GIT_VERSION}-Windows.zip"

# Issue 737: avoid clearing out old paths
# rm -rf "${distpath}"
# mkdir -p "${distpath}"
if [ ! -d ${distpath} ]; then
	mkdir -p "${distpath}"
fi

# Issue 794: 
# windeployqt copies all qt dependencies into the artifact bundle.
# However, non-qt dependencies are necessary, such as zlib.
# ldd copies all direct dependencies of the executable.
# sqlite3 somehow slips through the cracks: windeployqt correctly copies qsqlite3.dll,
# but misses the fact that qsqlite3.dll is only a symbolic link to libsqlite3-0.dll.
# ldd sees that libsqlite3-0.dll is not a direct dependency of the exectuable, so skips over
# this library.
# The simplest solution is just to manually copy over libsqlite3-0.dll

# [1] windeployqt: handles direct QT dependencies
echo "Preparing to run windeployqt.exe..."
windeployqt.exe "${exepath}" --dir "${distpath}"
echo "Successfully executed windeployqt.exe"

# [2] Ask the executable about non-qt dependencies, and copy them
# Issue 794: Explicitly avoid qt libs and plugins, letting windeployqt handle those.
for f in $(ldd "${exepath}" | awk '{print $3}' | grep 'mingw64' | grep -v 'Qt5' | grep -v 'plugins'); do
    b=${f##*/}
    cp -v "${f}" "${distpath}/${b}"
done

# [3] manually copy sqlite3, which is skipped over by both windeployqt and ldd.
echo "Manually copying libsqlite3-0.dll"
cp -v /c/msys64/mingw64/bin/libsqlite3-0.dll "${distpath}/"

mkdir -p "${distpath}/methods"
cp -v src/maven_core/bin/methods/* "${distpath}/methods"
# mkdir -p "${distpath}/pathways"
# cp -v bin/pathways/* "${distpath}/pathways"
mkdir -p "${distpath}/scripts"
cp -v src/maven_core/bin/scripts/* "${distpath}/scripts"
cp -v "${exepath}" "${distpath}/"

rm -rf "dist/${zipfn}"
(cd "${distpath}" && 7z a -tzip "../${zipfn}" *)

echo "make_dist_win32: All Processes Succcessfully Completed!"
