# use GB variable to express ownership intention and
# avoid conflict with documented and undocumented cmake variables

# FIXME: when we have a hierarchical build.
# Normally we would just set the version with the project command which
# would make it and its components available as variables.
# But, we aren't hierarchical yet, so we have multiple independent
# CMakeLists.txt files.  By defining the version here we have one
# less place to keep synchronized.
set(GB.VERSION 1.7.0)
string(REPLACE "." ";" VERSION_COMPONENTS ${GB.VERSION})
list(GET VERSION_COMPONENTS 0 GB.MAJOR)
list(GET VERSION_COMPONENTS 1 GB.MINOR)
list(GET VERSION_COMPONENTS 2 GB.MICRO)
# Increase GB.BUILD for a new release (why? Where is this ever used?)
# A: it's used by win32/gpsbabel.rc which includes gbversion.h
set(GB.BUILD 31 CACHE STRING "fourth component of windows VERSIONINFO resource FILEVERSION and PRODUCTVERSION parameters")
set(GB.PACKAGE_RELEASE "" CACHE STRING "string to append to VERSION tuple") # .e.g. "-beta20190413"

# may be overridden on cmake command line
set(DOCVERSION ${GB.VERSION} CACHE STRING "web server documentation URL version string (in WEB_DOC_DIR)")
