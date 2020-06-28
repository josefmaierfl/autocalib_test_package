#!/bin/bash

PY_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )/ngransac_prepare"
cd ${PY_DIR}
#echo "works $1"
python main.py "$@"
