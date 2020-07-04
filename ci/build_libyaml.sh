#!/usr/bin/env bash

root_dir=`pwd`
thirdparty_dir=${root_dir}/thirdparty

cd ${thirdparty_dir}
git clone https://github.com/yaml/libyaml.git
cd libyaml
./bootstrap
./configure
make
if [ $? -ne 0 ]; then
    exit 1
fi
make install
if [ $? -ne 0 ]; then
    exit 1
fi
#cd ..
#git clone https://github.com/yaml/pyyaml.git
#cd pyyaml
#python setup.py install
##pip install ruamel.yaml --upgrade --force
#if [ $? -ne 0 ]; then
#    exit 1
#fi
