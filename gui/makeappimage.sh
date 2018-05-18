#!/bin/sh
set -e -x
DISTDIR=dist
QMAKE=${QMAKE:-qmake}
LINUXDEPLOYQT=${LINUXDEPLOYQT:-linuxdeployqt}
rm -fr $DISTDIR
mkdir $DISTDIR
cp -p gpsbabelfe.desktop $DISTDIR
cp -p images/appicon.png $DISTDIR/gpsbabelfe.png
cp -p objects/gpsbabelfe-bin $DISTDIR/gpsbabelfe
if [ -x ../gpsbabel ]; then
  cp -p ../gpsbabel $DISTDIR/gpsbabel
elif [ -x ../GPSBabel ]; then
  cp -p ../GPSBabel $DISTDIR/gpsbabel
else
  echo "Couldn't find command line executable gpsbabel or GPSBabel." >&2
  exit 1;
fi
cp -p COPYING.txt $DISTDIR
cp -p gmapbase.html $DISTDIR
mkdir $DISTDIR/translations
cp -p *.qm $DISTDIR/translations
#https://github.com/probonopd/linuxdeployqt/issues/35
mkdir $DISTDIR/lib
cp -pr /usr/lib/x86_64-linux-gnu/nss $DISTDIR/lib/
$LINUXDEPLOYQT $DISTDIR/gpsbabelfe -qmake=$QMAKE -appimage -verbose=2 -executable=$DISTDIR/gpsbabel  2>&1 | tee makeappimage.log
