#!/bin/sh
set -ex

# Note that xmllint/xsltproc --path argument doesn't quite work because
# we have both formats/arc.xml and filters/arc.xml and xmllint/xsltproc
# can find the wrong one.

# Note that  xmllint/xsltproc XML_CATALOG_FILE is problematic on macos
# with macports or brew.  To avoid fetching from the internet we should
# append the default catalog /etc/xml/catalog to XML_CATALOG_FILE.
# However, on macos with macports or brew the default catalog is in
# another location.

if [ $# -eq 1 ]; then
  # pwd is assumed to be the build directory
  # pwd may or may not be the source directory
  sourcedir=$(realpath "$1")
  if [ "$sourcedir" != "$(realpath "$(pwd)")" ]; then
    "${sourcedir}/tools/gencatalog.sh" "${sourcedir}"
    xmlpath=$(dirname "$(command -v xmllint)")
    if [ "$xmlpath" = "/opt/local/bin" ]; then
      defcatalog=/opt/local/etc/xml/catalog
    elif [ "$xmlpath" = "/usr/local/bin" ]; then
      defcatalog=/usr/local/etc/xml/catalog
    else
      defcatalog=/etc/xml/catalog
    fi
    export XML_CATALOG_FILES="xmldoc/catalog.xml  $defcatalog"
  fi
else
  # pwd is assumed to be the build directory
  # pwd is assumed to be the source directory
  sourcedir=.
fi

perl "${sourcedir}/xmldoc/makedoc" xmldoc
xsltproc \
  --output gpsbabel.html \
  --stringparam toc.section.depth "1" \
  --stringparam html.cleanup "1" \
  --stringparam make.clean.html "1" \
  --stringparam html.valid.html "1" \
  --stringparam html.stylesheet "https://www.gpsbabel.org/style3.css" \
  http://docbook.sourceforge.net/release/xsl/current/xhtml/docbook.xsl \
  "${sourcedir}/xmldoc/readme.xml"
