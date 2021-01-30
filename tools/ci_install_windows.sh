#!/bin/bash
set -ex

# validate install
function validate() {
  (
    set +e
    # shellcheck source=/dev/null
    source "${CACHEDIR}/qt.env"
    if [ "$(cygpath -u "$(qmake -query QT_INSTALL_BINS)")" != "${QTDIR}/bin" ]; then
      echo "ERROR: unexpected Qt location." >&2
      exit 1
    fi
    if [ "$(qmake -query QT_VERSION)" != "${QT_VERSION}" ]; then
      echo "ERROR: wrong Qt version." >&2
      exit 1
    fi
  )
}

QT_VERSION=${1:-5.12.10}
COMPILER=${2:-msvc2017_64}
if [ "${COMPILER}" = "msvc2017_64" ]; then
  PACKAGE_SUFFIX=win64_msvc2017_64
elif [ "${COMPILER}" = "msvc2017" ]; then
  PACKAGE_SUFFIX=win32_msvc2017
elif [ "${COMPILER}" = "msvc2019_64" ]; then
  PACKAGE_SUFFIX=win64_msvc2019_64
elif [ "${COMPILER}" = "msvc2019" ]; then
  PACKAGE_SUFFIX=win32_msvc2019
else
  echo "ERROR: unrecognized Qt compiler ${COMPILER}." >&2
  exit 1
fi

CACHEDIR=${HOME}/Cache
QTDIR=${CACHEDIR}/Qt/${QT_VERSION}/${COMPILER}

if [ -d "${QTDIR}/bin" ]; then
  echo "Using cached Qt."
else
  rm -fr "${CACHEDIR}"
  python -mpip aqt install
  aqt install -O "${CACHEDIR}/Qt" ${QT_VERSION} windows desktop ${PACKAGE_SUFFIX} -m qtwebengine
  echo "export PATH=${QTDIR}/bin:\$PATH" > "${CACHEDIR}/qt.env"
fi
validate
