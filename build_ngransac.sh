#!/bin/bash

NGRANSAC_INST_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && cd .. && pwd )/matchinglib_poselib/source/poselib/thirdparty/ngransac/ngransac"
cd ${NGRANSAC_INST_DIR}

sudo python setup.py install
#sudo pip install -e .
