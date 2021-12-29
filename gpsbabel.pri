# Until we do a hierarchical build the build directory for gpsbabel
# and the build directory for GPSBabelFE are independent.
# Only the source directories have a known relationship.
# Including this pri file from the source tree will generate the version 
# file in the current build directory.
# Note some of these variables are also used in the gui to generate setup.iss.
# Note some of these variables are also used in GPSBabel.pro.
VERSION = 1.7.0

# use GB variable to express ownership intention and
# avoid conflict with documented and undocumented qmake variables
GB.VERSION_COMPONENTS = $$split(VERSION, .)
GB.VERSION = $${VERSION}
GB.MAJOR = $$member(GB.VERSION_COMPONENTS, 0)
GB.MINOR = $$member(GB.VERSION_COMPONENTS, 1)
GB.MICRO = $$member(GB.VERSION_COMPONENTS, 2)
# Increase GB.BUILD for a new release (why? Where is this ever used?)
# A: it's used by win32/gpsbabel.rc which includes gbversion.h
GB.BUILD = 31
# GB.PACKAGE_RELEASE = "-beta20190413"

# may be overridden on qmake command line
!defined(DOCVERSION, var) {
DOCVERSION=$${VERSION}
}

# use undocumented QMAKE_SUBSTITUTES variable to emulate AC_CONFIG_FILES
# Note $${PWD} is relative to the location of this file.
GB.versionfile.input = $${PWD}/gbversion.h.qmake.in
GB.versionfile.output = gbversion.h
QMAKE_SUBSTITUTES += GB.versionfile
