#!/bin/bash

root_dir=`pwd`
thirdparty_dir=${root_dir}/thirdparty

eval "$(conda shell.bash hook)"
conda activate NGRANSAC

cd ${thirdparty_dir}
git clone --recursive https://github.com/pytorch/pytorch
cd pytorch

export CUDA_NVCC_EXECUTABLE="/usr/local/cuda-10.2/bin/nvcc"
export CUDA_HOME="/usr/local/cuda-10.2"
export CUDNN_INCLUDE_PATH="/usr/local/cuda-10.2/include/"
export CUDNN_LIBRARY_PATH="/usr/local/cuda-10.2/lib64/"
export LIBRARY_PATH="/usr/local/cuda-10.2/lib64"

export CMAKE_PREFIX_PATH=${CONDA_PREFIX:-"$(dirname $(which conda))/../"}
export CFLAGS="-D_GLIBCXX_USE_CXX11_ABI=0 $CFLAGS"
export USE_CUDA=1 USE_CUDNN=1 USE_MKLDNN=1
python setup.py install

cd $thirdparty_dir
git clone git@github.com:pytorch/vision.git
cd vision
python setup.py install
