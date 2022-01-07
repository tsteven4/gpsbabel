#!/bin/sh
set -ex

if [ $# -eq 1 ]; then
  # pwd is assumed to be a build directory
  sourcedir=$1
  autogendir=.
  paths="autogen ${sourcedir}/xmldoc/formats ${sourcedir}/xmldoc/formats/options ${sourcedir}/xmldoc/filters ${sourcedir}/xmldoc/filters/options"
else
  # pwd is assumed to be the source directory
  sourcedir=.
  autogendir=xmldoc
  paths=
fi

perl "${sourcedir}/xmldoc/makedoc" "${autogendir}"
xmlwf "${sourcedir}/xmldoc/readme.xml" #check for well-formedness
xmllint --noout --valid --path "$paths" "${sourcedir}/xmldoc/readme.xml" #validate
xsltproc -o gpsbabel.fo --path "$paths" "${sourcedir}/xmldoc/babelpdf.xsl" "${sourcedir}/xmldoc/readme.xml"
HOME="${sourcedir}" fop -q -fo gpsbabel.fo -pdf gpsbabel.pdf
