/bin/bash -ex
QT_VERSION=${1:-5.12.10}

CACHEDIR=${HOME}/Cache
QTDIR=${CACHEDIR}/Qt/${QT_VERSION}/msvc2017_64
if [ -d "${QTDIR}/binxx" ]; then
  echo "Using cached Qt."
else
  rm -fr ${CACHEDIR}
  QT_VERSION_SHORT=${QT_VERSION//./}
  curl -s -L -o qt-opensource-windows-x86-${QT_VERSION}.exe "https://download.qt.io/archive/qt/5.12/${QT_VERSION}/qt-opensource-windows-x86-${QT_VERSION}.exe"
  netsh advfirewall firewall add rule name=dummyupqt dir=out action=block program="${PWD}/qt-opensource-windows-x86-${QT_VERSION}.exe"
  "${PWD}/qt-opensource-windows-x86-${QT_VERSION}.exe" --verbose --script "${CI_BUILD_DIR}/tools/qtci/qt-install.qs" QTCI_OUTPUT="${CACHEDIR}/Qt" QTCI_PACKAGES=qt.qt5.${QT_VERSION_SHORT}.win64_msvc2017_64,qt.qt5.${QT_VERSION_SHORT}.qtwebengine.win64_msvc2017_64
  netsh advfirewall firewall delete rule name=dummyupqt
  rm qt-opensource-windows-x86-${QT_VERSION}.exe
fi
"${QTDIR}/bin/qmake.exe" -v

