#!/usr/bin/env bash

CURR_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Name of application to install
CondaEnv="NGRANSAC"

CondaDepsFile="requirements_reinstall.txt"

eval "$(conda shell.bash hook)"
conda activate $CondaEnv

if [[ $CondaDepsFile ]]; then
    while read requirement; do conda install --yes -c defaults -c conda-forge -c anaconda $requirement || sudo pip install $requirement; done < "${CURR_DIR}/$CondaDepsFile"
fi
