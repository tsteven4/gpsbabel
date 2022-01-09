#!/bin/sh
set -ex

web=$1
docversion=$2

# Note that xmllint/xsltproc --path argument doesn't quite work because
# we have both formats/arc.xml and filters/arc.xml and xmllint/xsltproc
# can find the wrong one.

# Note that  xmllint/xsltproc XML_CATALOG_FILE is problematic on macos
# with macports or brew.  To avoid fetching from the internet we should
# append the default catalog /etc/xml/catalog to XML_CATALOG_FILE.
# However, on macos with macports or brew the default catalog is in
# another location.

if [ $# -eq 3 ]; then
  # pwd is assumed to be the build directory
  # pwd may or may not be the source directory
  sourcedir=$(realpath "$3")
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

mkdir -p "${web}/htmldoc-${docversion}"
perl "${sourcedir}/xmldoc/makedoc" xmldoc
xmlwf "${sourcedir}/xmldoc/readme.xml" #check for well-formedness
xmllint --noout --valid "${sourcedir}/xmldoc/readme.xml" #validate
xsltproc \
  --stringparam base.dir "${web}/htmldoc-${docversion}/" \
  --stringparam root.filename "index" \
  "${sourcedir}/xmldoc/babelmain.xsl" \
  "${sourcedir}/xmldoc/readme.xml"
"${sourcedir}/tools/fixdoc" "${web}/htmldoc-${docversion}" "GPSBabel ${docversion}:"
"${sourcedir}/tools/mkcapabilities" "${web}" "${web}/htmldoc-${docversion}"
cp gpsbabel.pdf "${web}/htmldoc-${docversion}/gpsbabel-${docversion}.pdf"
