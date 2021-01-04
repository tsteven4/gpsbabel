echo on
curl -s -L -o qt-opensource-windows-x86-5.12.10.exe "https://download.qt.io/archive/qt/5.12/5.12.10/qt-opensource-windows-x86-5.12.10.exe"
netsh advfirewall firewall add rule name=dummyupqt dir=out action=block program="%cd%\qt-opensource-windows-x86-5.12.10.exe"
"%cd%\qt-opensource-windows-x86-5.12.10.exe" --verbose --script "%CI_BUILD_DIR%\tools\qtci\qt-install.qs" QTCI_OUTPUT="%HOMEDRIVE%%HOMEPATH%\Cache\Qt" QTCI_PACKAGES=qt.qt5.51210.win64_msvc2017_64,qt.qt5.51210.qtwebengine.win64_msvc2017_64
netsh advfirewall firewall delete rule name=dummyupqt
dir "%HOMEDRIVE%%HOMEPATH%"
dir "%HOMEDRIVE%%HOMEPATH%\Cache"
dir "%HOMEDRIVE%%HOMEPATH%\Cache\Qt"
"%HOMEDRIVE%%HOMEPATH%\Cache\Qt\5.12.10\msvc2017_64\bin\qmake.exe" -v

