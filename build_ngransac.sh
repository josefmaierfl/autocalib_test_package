#!/usr/bin/env bash

NGRANSAC_INST_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )/matchinglib_poselib/source/poselib/thirdparty/ngransac/ngransac"
cd ${NGRANSAC_INST_DIR}

echo $HOME
echo $PATH
echo $USER
#eval "$(conda shell.bash hook)"
#conda activate NGRANSAC

sudo python setup.py install
#sudo pip install -e .
