#!/bin/sh
set -ex

web=${1:-gpsbabel.org}
docversion=${2:-x.y.z}
sourcedir=$(cd "$(dirname "$0")/.." && pwd)

mkdir -p "${web}/htmldoc-${docversion}"
"$sourcedir"/tools/make_gpsbabel_doc.sh
xsltproc \
  --stringparam base.dir "${web}/htmldoc-${docversion}/" \
  --stringparam root.filename "index" \
  "${sourcedir}/xmldoc/babelmain.xsl" \
  xmldoc/readme.xml
"${sourcedir}"/tools/fixdoc "${web}/htmldoc-${docversion}" "GPSBabel ${docversion}:"
"${sourcedir}"/tools/mkcapabilities "${web}" "${web}/htmldoc-${docversion}"
cp gpsbabel.pdf "${web}/htmldoc-${docversion}/gpsbabel-${docversion}.pdf"
