#!/bin/bash
set -ex

# This is a hack to get qtwebengine, qtpdf with 6.8.0.
git clone https://github.com/tsteven4/aqtinstall
cd aqtinstall
git checkout gpsbabel
python3 -m pip install build
python3 -m build
python3 -m pip install ./dist/aqtinstall-*-py3-none-any.whl
