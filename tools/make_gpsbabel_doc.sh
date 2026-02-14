#!/bin/sh
set -ex

perl xmldoc/makedoc
# with xmllint 21501 we have to process entities explicitly and before relaxng verification.
# with xmllint 20914 this wasn't necessary.
#xmllint --noout --relaxng http://docbook.org/xml/5.0/rng/docbook.rng xmldoc/readme.xml
xmllint --noent xmldoc/readme.xml | xmllint --noout --relaxng http://docbook.org/xml/5.0/rng/docbook.rng -
# the following doesn't seem to work.
#xmllint --noout --schematron http://docbook.org/xml/5.0/sch/docbook.sch xmldoc/readme.xml
# jing and many depedencies removed from fedora
if command -v jing >/dev/null 2>&1; then
  jing tools/schema/docbook/xml/5.0/rng/docbook.rng xmldoc/readme.xml
  # can seed a failure by removing version="5.0" from xmldoc/readme.xml
  jing tools/schema/docbook/xml/5.0/sch/docbook.sch xmldoc/readme.xml
fi
