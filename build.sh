#!/usr/bin/env bash

comp_dir=compile
orig_dir=`pwd`

cd `dirname "$0"`
if [[ ! -e $comp_dir ]]; then
    mkdir $comp_dir
fi
cd $comp_dir
cmake ..
make
cd $orig_dir
