#!/usr/bin/env bash

NGRANSAC_INST_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )/matchinglib_poselib/source/poselib/thirdparty/ngransac/ngransac"
cd ${NGRANSAC_INST_DIR}
# source ~/.bashrc
# echo $HOME
# echo $PATH
# echo $USER
eval "$(conda shell.bash hook)"
conda activate NGRANSAC
#conda list

export CFLAGS="-D_GLIBCXX_USE_CXX11_ABI=1 $CFLAGS"
python setup.py install
#sudo pip install -e .
if [ $? -ne 0 ]; then
    exit 1
fi