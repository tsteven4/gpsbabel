#!/bin/bash
set -ex

# This is a hack to get qtwebengine, qtpdf with 6.8.0.
git clone https://github.com/tsteven4/aqtinstall
cd aqtinstall
python3 -m pipx install build
python3 -m build
python3 -m pipx install ./dist/aqtinstall-*-py3-none-any.whl
