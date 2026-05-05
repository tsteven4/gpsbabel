#!/bin/bash -ex
tmpfile=$(mktemp --suffix .svg)
inkscape --actions="file-open:$(pwd)/images/appicon_master.svg;select-by-id:image1;delete;export-plain-svg;export-filename:${tmpfile};export-do;file-close"
snap run --shell inkscape <<EOJ
scour --enable-id-stripping -i ${tmpfile} -o "$(pwd)/images/appicon.svg"
EOJ
rm "${tmpfile}"

commondims=(16 22 24 32 48 64 128 256)
for dim in "${commondims[@]}"
do
  dims=${dim}x${dim}
  inkscape --export-type="png" -w "${dim}" -h "${dim}" "$(pwd)/images/appicon.svg" -o "$(pwd)/images/appicon-${dims}.png"
done
