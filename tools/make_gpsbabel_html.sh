#!/bin/sh
set -ex

sourcedir=$(cd "$(dirname "$0")/.." && pwd)

mkdir -p gpsbabel.html
"$sourcedir"/tools/make_gpsbabel_doc.sh
# the img.src.path must allow the images to be found on the web server.
xsltproc \
  --output gpsbabel.html/index.html \
  --stringparam toc.section.depth "1" \
  --stringparam html.cleanup "1" \
  --stringparam make.clean.html "1" \
  --stringparam html.valid.html "1" \
  --stringparam html.stylesheet "https://www.gpsbabel.org/style3.css" \
  --stringparam img.src.path "images/" \
  http://docbook.sourceforge.net/release/xsl-ns/current/xhtml/docbook.xsl \
  xmldoc/readme.xml
cp -pr "${sourcedir}/xmldoc/images" gpsbabel.html/images
