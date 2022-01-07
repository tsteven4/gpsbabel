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
xsltproc \
  --output gpsbabel.html \
  --path "${paths}" \
  --stringparam toc.section.depth "1" \
  --stringparam html.cleanup "1" \
  --stringparam make.clean.html "1" \
  --stringparam html.valid.html "1" \
  --stringparam html.stylesheet "https://www.gpsbabel.org/style3.css" \
  http://docbook.sourceforge.net/release/xsl/current/xhtml/docbook.xsl \
  ${sourcedir}/xmldoc/readme.xml
