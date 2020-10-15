#!/bin/bash

# RES_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && cd .. && pwd )/results/results_001/results"
# OUTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && cd .. && pwd )/python_tests_out"
RES_SV_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && cd .. && pwd )/res_save_compressed"
if [ ! -d ${RES_SV_DIR} ]; then
  mkdir ${RES_SV_DIR}
fi

if [ $# -eq 0 ]; then
  echo "Arguments are required"
  exit 1
fi
FIRST_ARG="$1"
if [ "${FIRST_ARG}" == "shutdown" ]; then
  echo "Shutting down after calculations finished"
elif [ "${FIRST_ARG}" == "live" ]; then
  echo "Keeping system alive after calculations finished"
else
  echo "First argument must be shutdown or live"
  exit 1
fi
shift 1

USE_FULL_PATH=0
if [ "$1" == "fullp" ]; then
  USE_FULL_PATH=1
  shift 1
fi

if [ $# -ge 2 ]; then
  SECOND_ARG="$1"
  SECOND_ARG1="$2"
else
  SECOND_ARG=""
fi
if [ $# -ge 4 ]; then
  THIRD_ARG="$3"
  THIRD_ARG1="$4"
else
  THIRD_ARG=""
fi
if [ "${SECOND_ARG}" == "EXE" ] && [ "${THIRD_ARG}" == "RESDIR" ]; then
  SECOND_ARG="RESDIR"
  THIRD_ARG="EXE"
  TMP=${SECOND_ARG1}
  SECOND_ARG1=${THIRD_ARG1}
  THIRD_ARG1=${TMP}
elif [ "${SECOND_ARG}" == "EXE" ]; then
  THIRD_ARG="EXE"
  THIRD_ARG1=${SECOND_ARG1}
  SECOND_ARG=""
fi
if [ "${SECOND_ARG}" == "RESDIR" ]; then
  if [ ${USE_FULL_PATH} -eq 0 ]; then
    RES_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && cd .. && pwd )/${SECOND_ARG1}"
  else
    RES_DIR="${SECOND_ARG1}"
  fi
  if [ -d ${RES_DIR} ]; then
    shift 2
  else
    echo "Given directory for storing results does not exist"
    exit 1
  fi
else
  RES_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && cd .. && pwd )/results"
  if [ ! -d ${RES_DIR} ]; then
    mkdir ${RES_DIR}
  fi
fi

if [ "${THIRD_ARG}" == "EXE" ]; then
  if [ "${THIRD_ARG1}" == "train" ]; then
    SCRIPT="start_training.sh"
    RES_DIRD="/app/results_train"
    shift 2
  elif [ "${THIRD_ARG1}" == "test" ]; then
    SCRIPT="start_testing.sh"
    RES_DIRD="/app/results"
    shift 2
  else
    echo "Parameter EXE must follow train or test"
    exit 1
  fi
else
  SCRIPT="start_testing.sh"
  RES_DIRD="/app/results"
fi
# -c $(echo "${@:2}")
xhost +local:
#docker run -v `pwd`/py_test_scripts:/app/py_test_scripts -it -v /tmp/.X11-unix/:/tmp/.X11-unix:ro ac_test_package:1.0 /bin/bash
# docker run --gpus all -v `pwd`/images:/app/images:ro -v `pwd`/py_test_scripts:/app/py_test_scripts -v ${OUTDIR}:/app/output -v ${RES_DIR}:/app/results -v ${RES_SV_DIR}:/app/res_save_compressed -it -v /tmp/.X11-unix/:/tmp/.X11-unix:ro ac_test_package_ngransac:1.0 /bin/bash
# docker run --gpus all -v `pwd`/images:/app/images:ro -v `pwd`/py_test_scripts:/app/py_test_scripts -v `pwd`/ngransac_train:/app/ngransac_train -v ${RES_DIR}:${RES_DIRD} -v ${RES_SV_DIR}:/app/res_save_compressed -it -v /tmp/.X11-unix/:/tmp/.X11-unix:ro ac_test_package_ngransac:1.0 /bin/bash
docker run --gpus all -v `pwd`/images:/app/images:ro -v `pwd`/py_test_scripts:/app/py_test_scripts -v `pwd`/ngransac_train:/app/ngransac_train -v ${RES_DIR}:${RES_DIRD} -v ${RES_SV_DIR}:/app/res_save_compressed -it -v /tmp/.X11-unix/:/tmp/.X11-unix:ro ac_test_package_ngransac:1.0 /app/${SCRIPT} "$@"

# Shut down if asked for
#if [ $# -ne 0 ]; then
    if [ "${FIRST_ARG}" == "shutdown" ]; then
        echo "Shutting down"
        sudo shutdown -h now
    fi
#fi
