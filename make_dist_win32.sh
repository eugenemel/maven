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

GIT_VERSION=$(src/maven/get_version.sh)

exepath=$1
exefn="${exepath##*/}"
distpath="dist/${exefn%.exe}-Windows"
zipfn="${exefn%.exe}_${GIT_VERSION}-Windows.zip"

rm -rf "${distpath}"
mkdir -p "${distpath}"
windeployqt.exe "${exepath}" --dir "${distpath}/${b}"

# Issue 540: Add peakdetector executable
# windeployqt.exe "src/maven/bin/peakdetector.exe" --dir "${distpath}/${b}"

for f in $(ldd "${exepath}" | awk '{print $3}' | grep 'mingw64'); do
    b=${f##*/}
    cp -v "${f}" "${distpath}/${b}"
done
mkdir -p "${distpath}/methods"
cp -v src/maven_core/bin/methods/* "${distpath}/methods"
# mkdir -p "${distpath}/pathways"
# cp -v bin/pathways/* "${distpath}/pathways"
mkdir -p "${distpath}/scripts"
cp -v src/maven_core/bin/scripts/* "${distpath}/scripts"
cp -v "${exepath}" "${distpath}/"

#Issue 499: Overwrite bad qsqlite.dll file with working qsqlite.dll file
cp src/maven_core/bin/dll/qsqlite.dll "${distpath}"/sqldrivers

#Issue 540: mzDeltas does not work with windeployqt, copy executable directly
# cp -v src/maven_core/bin/mzDeltas.exe "${distpath}"

#Issue 540: Copy peakdetector executable to final location
# cp -v "src/maven/bin/peakdetector.exe" "${distpath}/"

rm -rf "dist/${zipfn}"
(cd "${distpath}" && 7z a -tzip "../${zipfn}" *)
