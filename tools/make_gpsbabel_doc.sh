#!/bin/sh
set -ex

sourcedir=$(cd "$(dirname "$0")/.." && pwd)

perl "${sourcedir}/xmldoc/makedoc"
xmllint --noout --relaxng "file:///${sourcedir}/tools/schema/docbook/xml/5.0/rng/docbook.rng" xmldoc/readme.xml
# the following doesn't seem to work.
#xmllint --noout --schematron http://docbook.org/xml/5.0/sch/docbook.sch xmldoc/readme.xml
# jing and many depedencies removed from fedora
if command -v jing >/dev/null 2>&1; then
  jing "${sourcedir}/tools/schema/docbook/xml/5.0/rng/docbook.rng" xmldoc/readme.xml
  # can seed a failure by removing version="5.0" from xmldoc/readme.xml
  jing "${sourcedir}/tools/schema/docbook/xml/5.0/sch/docbook.sch" xmldoc/readme.xml
fi
