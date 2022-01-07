#!/bin/sh
set -ex

web=$1
docversion=$2

if [ $# -eq 3 ]; then
  # pwd is assumed to be a build directory
  sourcedir=$3
  autogendir=.
  paths="autogen ${sourcedir}/xmldoc/formats ${sourcedir}/xmldoc/formats/options ${sourcedir}/xmldoc/filters ${sourcedir}/xmldoc/filters/options"
else
  # pwd is assumed to be the source directory
  sourcedir=.
  autogendir=xmldoc
  paths=
fi

mkdir -p "${web}/htmldoc-${docversion}"
perl "${sourcedir}/xmldoc/makedoc" "${autogendir}"
xmlwf "${sourcedir}/xmldoc/readme.xml" #check for well-formedness
xmllint --noout --path "${paths}" --valid "${sourcedir}/xmldoc/readme.xml" #validate
xsltproc \
  --path "$paths" \
  --stringparam base.dir "${web}/htmldoc-${docversion}/" \
  --stringparam root.filename "index" \
  "${sourcedir}/xmldoc/babelmain.xsl" \
  "${sourcedir}/xmldoc/readme.xml"
"${sourcedir}/tools/fixdoc" "${web}/htmldoc-${docversion}" "GPSBabel ${docversion}:"
"${sourcedir}/tools/mkcapabilities" "${web}" "${web}/htmldoc-${docversion}"
cp gpsbabel.pdf "${web}/htmldoc-${docversion}/gpsbabel-${docversion}.pdf"
