#!/bin/bash

RES_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && cd .. && pwd )/results"
mkdir ${RES_DIR}
# RES_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && cd .. && pwd )/results/results_001/results"
# OUTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && cd .. && pwd )/python_tests_out"
RES_SV_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && cd .. && pwd )/res_save_compressed"
mkdir ${RES_SV_DIR}

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
# -c $(echo "${@:2}")
xhost +local:
#docker run -v `pwd`/py_test_scripts:/app/py_test_scripts -it -v /tmp/.X11-unix/:/tmp/.X11-unix:ro ac_test_package:1.0 /bin/bash
# docker run --gpus all -v `pwd`/images:/app/images:ro -v `pwd`/py_test_scripts:/app/py_test_scripts -v ${OUTDIR}:/app/output -v ${RES_DIR}:/app/results -v ${RES_SV_DIR}:/app/res_save_compressed -it -v /tmp/.X11-unix/:/tmp/.X11-unix:ro ac_test_package_ngransac:1.0 /bin/bash
docker run --gpus all -v `pwd`/images:/app/images:ro -v `pwd`/py_test_scripts:/app/py_test_scripts -v ${RES_DIR}:/app/results -v ${RES_SV_DIR}:/app/res_save_compressed -it -v /tmp/.X11-unix/:/tmp/.X11-unix:ro ac_test_package_ngransac:1.0 /app/start_testing.sh "${@:2}"

# Shut down if asked for
if [ $# -ne 0 ]; then
    if [ "${FIRST_ARG}" == "shutdown" ]; then
        echo "Shutting down"
        sudo shutdown -h now
    fi
fi
