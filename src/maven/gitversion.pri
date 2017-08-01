# Original author: Dmitriy Kubyshkin <grassator@gmail.com>
# https://www.everythingfrontend.com/posts/app-version-from-git-tag-in-qt-qml.html

# If there is no version tag in git this one will be used
VERSION = 0.1.0

# Need to discard STDERR so get path to NULL device
win32 {
    NULL_DEVICE = NUL # Windows doesn't have /dev/null but has NUL
} else {
    NULL_DEVICE = /dev/null
}

# Need to call git with manually specified paths to repository
#BASE_GIT_COMMAND = git --git-dir $$PWD/.git --work-tree $$PWD
BASE_GIT_COMMAND = git

# Get version from environment variable (available to downstream scripts too)
GIT_VERSION = $$(GIT_VERSION)
isEmpty(GIT_VERSION) {
    # Trying to get version from git tag / revision
    GIT_VERSION = $$system($$BASE_GIT_COMMAND describe --always --tags 2> $$NULL_DEVICE)
}

# Turns describe output like 0.1.5-42-g652c397 into "0.1.5.42.652c397"
GIT_VERSION ~= s/-/"."
GIT_VERSION ~= s/g/""

# Now we are ready to pass parsed version to Qt
VERSION = $$GIT_VERSION
win32 { # On windows version can only be numerical so remove commit hash
    VERSION ~= s/\.\d+\.[a-f0-9]{6,}//
}

# Adding C preprocessor #DEFINE so we can use it in C++ code
# also here we want full version on every system so using GIT_VERSION
DEFINES += GIT_VERSION=\\\"$$GIT_VERSION\\\"


# By default Qt only uses major and minor version for Info.plist on Mac.
# This will rewrite Info.plist with full version
# TODO Improve plist file (poor metadata)
# This currently doesn't appear in our plist file
#macx {
#    INFO_PLIST_PATH = $$shell_quote($${OUT_PWD}/$${INSTALL_ROOT}/$${target.path}/$${TARGET}.app/Contents/Info.plist)
#    QMAKE_POST_LINK += /usr/libexec/PlistBuddy -c \"Set :CFBundleShortVersionString $${VERSION}\" $${INFO_PLIST_PATH}
}#
