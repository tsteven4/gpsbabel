#!/bin/sh
set -ex

sourcedir=$(cd "$(dirname "$0")/.." && pwd)

"$sourcedir"/tools/make_gpsbabel_doc.sh
# the img.src.path must allow the images to be found when create the pdf.
xsltproc \
  -o gpsbabel.fo \
  --stringparam img.src.path "${sourcedir}/xmldoc/images/" \
  "${sourcedir}/xmldoc/babelpdf.xsl" xmldoc/readme.xml
HOME="${sourcedir}" fop -q -fo gpsbabel.fo -pdf gpsbabel.pdf
