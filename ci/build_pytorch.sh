#!/bin/bash

root_dir=`pwd`
thirdparty_dir=${root_dir}/thirdparty

eval "$(conda shell.bash hook)"
conda activate NGRANSAC

# wget https://github.com/ninja-build/ninja/releases/download/v1.10.0/ninja-linux.zip
# sudo unzip ninja-linux.zip -d /usr/local/bin/
# sudo update-alternatives --install /usr/bin/ninja ninja /usr/local/bin/ninja 1 --force

cd ${thirdparty_dir}
# git clone --recursive https://github.com/pytorch/pytorch
cd pytorch

# export CUDA_NVCC_EXECUTABLE="/usr/local/cuda-10.1/bin/nvcc"
export CUDA_HOME="/usr/local/cuda-10.1"
# export CUDNN_INCLUDE_PATH="/usr/local/cuda-10.1/include/"
export CUDNN_LIBRARY_PATH="/usr/local/cuda-10.1/lib64/"
export LIBRARY_PATH="/usr/local/cuda-10.1/lib64"

export CMAKE_PREFIX_PATH=${CONDA_PREFIX:-"$(dirname $(which conda))/../"}
export CFLAGS="-D_GLIBCXX_USE_CXX11_ABI=0 $CFLAGS"
export TORCH_CUDA_ARCH_LIST="3.5 5.2 6.0 6.1 7.0+PTX"
export TORCH_NVCC_FLAGS="-Xfatbin -compress-all"
export USE_CUDA=1 USE_CUDNN=1 USE_MKLDNN=1 USE_OPENCV USE_GLOO=0
python setup.py install

# cd $thirdparty_dir
# git clone git@github.com:pytorch/vision.git
# cd vision
# python setup.py install
