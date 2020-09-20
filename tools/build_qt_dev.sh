#!/bin/bash -ex

#if [ $# -ne 1 ]; then
#  echo "$0 version"
#  exit 1
#fi
version=dev

scriptdir="$(cd "$(dirname "${BASH_SOURCE[0]}" )" && pwd)"
sourcetype=git
#flavor=debug
flavor=release
#flavor=debug-and-release
features=(-feature-textcodec)

buildroot=$(pwd)/qt-${version}-${flavor}-${sourcetype}
mkdir "$buildroot"
sourcedir=${buildroot}/source
builddir=${buildroot}/build
if [ "$(uname -s)" = "Darwin" ]; then
  installdir=/Users/travis/Cache/Qt
  compilerdir=clang_64
  archive=qt-${version}-${flavor}-macos
  parallel=-j4
  #export PATH=/usr/local/Cellar/llvm/10.0.1_1/bin:$PATH
  #which llvm-config
else
  installdir=/home/travis/Cache/Qt
  compilerdir=gcc_64
  archive=qt-${version}-${flavor}-linux
  parallel=-j2
  features+=(-feature-xcb)
fi


if [ -e "${sourcedir}" ]; then
  echo "source directory \"${sourcedir}\" already exits."
  exit 1
fi
if [ -e "${builddir}" ]; then
  echo "build directory \"${builddir}\" already exits."
  exit 1
fi
if [ -e "${installdir}" ]; then
  echo "install directory \"${installdir}\" already exits."
  exit 1
fi

if [ "${sourcetype}" == "git" ]; then
  git clone git://code.qt.io/qt/qt5.git "${sourcedir}"
else
  mkdir -p "${sourcedir}"
  versionmm=$(echo "${version}" | cut -d. -f1,2)
  if [ ! -e "qt-everywhere-src-${version}.tar.xz" ]; then
    wget -nv "https://download.qt.io/archive/qt/${versionmm}/${version}/single/qt-everywhere-src-${version}.tar.xz"
  fi
  tar -x --strip-components 1 --xz --directory "${sourcedir}" --file "qt-everywhere-src-${version}.tar.xz"
fi
cd "${sourcedir}"

# exclude modules without some kind of LGPL license.
# virtualkeyboard is pretty sticky, it gets deployed if it exists and you use Gui.
excludes=( \
qtcharts \
qtdatavis3d \
qtlottie \
qtnetworkauth \
qtquick3d \
qtvirtualkeyboard \
qtwebglplugin \
)

# also some modules we don't use
excludes+=( \
qtpurchasing \
qtscript \
qtxmlpatterns \
)

# other modules we don't want
# the tarballs don't include these modules, but git does
excludes+=( \
qtqa \
qtwebengine \
qtwebchannel \
qtscript \
qtdeclarative \
qtquickcontrols2 \
qtimageformats \
qtshadertools \
qtsvg \
qtwayland \
qt3d \
)

#qttools \
#qttranslations \

if [ "${sourcetype}" == "git" ]; then
  if false; then
    # tagged when released
    git checkout "v${version}"
  else
    # branch, before tagged
    git checkout "${version}"
  fi
  modules=essential,addon,qt5compat,-$(echo "${excludes[@]}" | sed 's/ /,-/g')
  echo "$modules"
  perl init-repository --module-subset="${modules}"

  echo "Patching qttools"
  cd qttools
  git apply ${scriptdir}/qttools.patch
  cd ..

  gitrev=$(git rev-parse --short HEAD)
else
  for component in "${excludes[@]}"
  do
    /bin/rm -fr "${component}"
  done
fi

mkdir -p "${builddir}"
cd "${builddir}"
"${sourcedir}/configure" --prefix="${installdir}/${version}/${compilerdir}" -opensource -confirm-license -nomake examples -nomake tests "-${flavor}" "${features[@]}"
make ${parallel}
make install

licenses=( \
"${sourcedir}/LICENSE.FDL"  \
"${sourcedir}/LICENSE.GPLv2" \
"${sourcedir}/LICENSE.GPLv3" \
"${sourcedir}/LICENSE.LGPLv21" \
"${sourcedir}/LICENSE.LGPLv3" \
"${sourcedir}/LICENSE.QT-LICENSE-AGREEMENT" \
)

mkdir "${installdir}/Licenses"
for license in "${licenses[@]}"
do
  cp "${license}" "${installdir}/Licenses"
done

tar -c -C "$(dirname ${installdir})" --xz -f "${archive}-${gitrev}.tar.xz" "$(basename ${installdir})"
