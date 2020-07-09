#!/usr/bin/env bash

PY_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )/ngransac_train"
cd ${PY_DIR}
NGRANSAC_FILE="${PY_DIR}/dataset.py"
if [ ! -s ${NGRANSAC_FILE} ]; then
  ./prepare_workspace.sh
fi
eval "$(conda shell.bash hook)"
conda activate NGRANSAC
export MKL_THREADING_LAYER=TBB
#echo "works $1"
python main_train.py "$@"
