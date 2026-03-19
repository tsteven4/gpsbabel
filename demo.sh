#!/bin/bash -ex
cmake -G Ninja -DCMAKE_PREFIX_PATH=/opt/Qt/6.10.2/gcc_64 -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=bld/testinstall -B bld
cmake --build bld
cmake --install bld
ls bld/testinstall
