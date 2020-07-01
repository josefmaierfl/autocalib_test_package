#!/usr/bin/env bash

PY_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )/ngransac_train"
cd ${PY_DIR}
eval "$(conda shell.bash hook)"
conda activate NGRANSAC
export MKL_THREADING_LAYER=TBB
#echo "works $1"
python main.py "$@"
