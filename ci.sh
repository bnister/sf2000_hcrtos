#!/bin/sh
./install-dependencies.sh
./install-toolchain.sh
git submodules update --init
./buildSfClean.sh
