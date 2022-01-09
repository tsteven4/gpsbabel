#!/bin/bash

if [ $# -ne 1 ]; then
  echo "Usage: $0 sourcedir"
  exit 1
fi
sourcedir=$(realpath "$1")
binarydir=$(pwd)

xmlcatalog --noout --create xmldoc/catalog.xml
xmlcatalog --noout --add rewriteSystem "${sourcedir}/xmldoc/autogen/" "file://${binarydir}/xmldoc/autogen/" xmldoc/catalog.xml
xmlcatalog --noout --add rewriteSystem "file://${binarydir}/xmldoc/formats/" "file://${sourcedir}/xmldoc/formats/" xmldoc/catalog.xml
xmlcatalog --noout --add rewriteSystem "file://${binarydir}/xmldoc/filters/" "file://${sourcedir}/xmldoc/filters/" xmldoc/catalog.xml
