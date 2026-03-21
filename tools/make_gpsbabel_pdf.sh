#!/bin/sh
set -ex

sourcedir=$(cd "$(dirname "$0")/.." && pwd)

"$sourcedir"/tools/make_gpsbabel_doc.sh
xsltproc -o gpsbabel.fo "${sourcedir}/xmldoc/babelpdf.xsl" xmldoc/readme.xml
HOME=. fop -q -fo gpsbabel.fo -pdf gpsbabel.pdf
