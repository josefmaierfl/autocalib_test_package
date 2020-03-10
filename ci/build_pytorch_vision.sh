#!/bin/bash

root_dir=`pwd`
thirdparty_dir=${root_dir}/thirdparty

eval "$(conda shell.bash hook)"
conda activate NGRANSAC

cd ${thirdparty_dir}
# git clone https://github.com/pytorch/vision.git
cd vision

export CFLAGS="-D_GLIBCXX_USE_CXX11_ABI=1 $CFLAGS"
python setup.py install
