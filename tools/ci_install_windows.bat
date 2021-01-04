echo on
SETLOCAL
SET CACHEDIR=%HOMEDRIVE%%HOMEPATH%\Cache
SET QTDIR=%CACHEDIR%\Qt\%QT_VERSION%\msvc2017_64
IF EXIST "%CACHEDIR%\bin" (
  echo "Using cached Qt."
) ELSE (
  DEL /S "%CACHEDIR%"
  curl -s -L -o qt-opensource-windows-x86-%QT_VERSION%.exe "https://download.qt.io/archive/qt/5.12/%QT_VERSION%/qt-opensource-windows-x86-%QT_VERSION%.exe"
  netsh advfirewall firewall add rule name=dummyupqt dir=out action=block program="%cd%\qt-opensource-windows-x86-%QT_VERSION%.exe"
  "%cd%\qt-opensource-windows-x86-%QT_VERSION%.exe" --verbose --script "%CI_BUILD_DIR%\tools\qtci\qt-install.qs" QTCI_OUTPUT="%CACHEDIR%\Qt" QTCI_PACKAGES=qt.qt5.%QT_VERSION:.=%.win64_msvc2017_64,qt.qt5.%QT_VERSION:.=%.qtwebengine.win64_msvc2017_64
  netsh advfirewall firewall delete rule name=dummyupqt
  DEL qt-opensource-windows-x86-%QT_VERSION%.exe
  DIR "%HOMEDRIVE%%HOMEPATH%"
  DIR "%CACHEDIR%
  DIR "%QTDIR%
  "%CACHEDIR%\bin\qmake.exe" -v
)
ENDLOCAL

