#!/bin/bash

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

SECOND_ARG="$1"
if [ "${SECOND_ARG}" == "RESDIR" ]; then
  if [ ${USE_FULL_PATH} -eq 0 ]; then
    RES_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && cd .. && pwd )/$2"
  else
    RES_DIR="$2"
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
# -c $(echo "${@:2}")
xhost +local:
#docker run -v `pwd`/images:/app/images:ro -v `pwd`/py_test_scripts:/app/py_test_scripts -v ${RES_DIR}:/app/results -v ${RES_SV_DIR}:/app/res_save_compressed -it -v /tmp/.X11-unix/:/tmp/.X11-unix:ro ac_test_package:1.0 /bin/bash
docker run -v `pwd`/images:/app/images:ro -v `pwd`/py_test_scripts:/app/py_test_scripts -v ${RES_DIR}:/app/results -v ${RES_SV_DIR}:/app/res_save_compressed -it -v /tmp/.X11-unix/:/tmp/.X11-unix:ro ac_test_package:1.0 /app/start_testing.sh "$@"

# Shut down if asked for
#if [ $# -ne 0 ]; then
    if [ "${FIRST_ARG}" == "shutdown" ]; then
        echo "Shutting down"
        sudo shutdown -h now
    fi
#fi
