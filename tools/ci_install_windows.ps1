C:\Windows\System32\curl -L -o qt-opensource-windows-x86-5.12.10.exe "https://download.qt.io/archive/qt/5.12/5.12.10/qt-opensource-windows-x86-5.12.10.exe"
netsh advfirewall firewall add rule name=dummyupqt dir=out action=block program="$Pwd\qt-opensource-windows-x86-5.12.10.exe"
& "$Pwd\qt-opensource-windows-x86-5.12.10.exe" --verbose --platform minimal --script qt-install.qs QTCI_OUTPUT="$Home\Cache\Qt" QTCI_PACKAGES=qt.qt5.51210.win64_msvc2017_64,qt.qt5.51210.qtwebengine.win64_msvc2017_64
netsh advfirewall firewall delete rule name=dummyupqt
& "$Home\Cache\Qt\5.12.10\msvc2017_64\bin\qmake" -v

