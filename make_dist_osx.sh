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
    echo -e \\n"${REV}Basic usage:${NORM} ${BOLD}$SCRIPT Maven.app${NORM}"\\n
    echo -e "${REV}-h${NORM}  --Displays this help message. No further functions are performed."\\n
    echo -e "Example: ${BOLD}$SCRIPT \"${apppath}\"${NORM}"\\n
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

apppath=$1
basepath="${apppath%.app}"
appfn="${apppath##*/}"
distpath="dist"
basefn="${appfn%.app}"
# Get version
VERSION=$("src/maven/get_version.sh")
dmgfn="${basefn}_${VERSION}.dmg"

rm -rf "${distpath}"
mkdir -p "${distpath}"

echo "Copying resources to ${apppath}"
mkdir -p "${apppath}/Contents/Resources/methods"
cp src/maven_core/bin/methods/* "${apppath}/Contents/Resources/methods"
mkdir -p "${apppath}/Contents/Resources/scripts"
cp src/maven_core/bin/scripts/* "${apppath}/Contents/Resources/scripts"
# TODO Should pathways be in the repo?
# mkdir "${apppath}/Contents/Resources/pathways"
# cp bin/pathways/* "${apppath}/Contents/Resources/pathways"
# TODO Should these be in the repo?
# cp bin/*.csv   "${apppath}/Contents/Resources"
# cp bin/*.model "${apppath}/Contents/Resources"

echo "Running macdeployqt"
macdeployqt "${apppath}"
# TODO This doesn't appear to get built, is that correct or are we missing something?
# macdeployqt "bin/peakdetector.app" -dmg

echo "Making DMG"
hdiutil create "${distpath}/${dmgfn}" -srcfolder "${apppath}" -ov

