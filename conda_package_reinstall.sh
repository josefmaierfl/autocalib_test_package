#!/bin/bash

CURR_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Name of application to install
CondaEnv="NGRANSAC"

CondaDepsFile="requirements_reinstall.txt"
CondaDepsFilePip="requirements_pip.txt"

eval "$(conda shell.bash hook)"
conda activate $CondaEnv

if [[ $CondaDepsFile ]]; then
  conda install --yes -c defaults -c conda-forge -c anaconda --file "${CURR_DIR}/$CondaDepsFile"
    #while read requirement; do conda install --yes -c defaults -c conda-forge -c anaconda $requirement || sudo pip install $requirement; done < "${CURR_DIR}/$CondaDepsFile"
fi
if [[ $CondaDepsFilePip ]]; then
    while read requirement; do pip install $requirement; done < "${CURR_DIR}/$CondaDepsFilePip"
fi
