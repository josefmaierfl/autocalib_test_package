#!/bin/bash

root_dir=`pwd`
thirdparty_dir=${root_dir}/thirdparty
mkdir ${thirdparty_dir}

#-----------------------------------
# Eigen
#-----------------------------------

cd ${root_dir}
./build_eigen.sh

if [ $? -ne 0 ]; then
    exit 1
fi

#-----------------------------------
# Boost
#-----------------------------------
cd ${root_dir}
./build_boost.sh

if [ $? -ne 0 ]; then
    exit 1
fi
ldconfig

#-----------------------------------
# VTK
#-----------------------------------

cd ${root_dir}
./build_vtk.sh

if [ $? -ne 0 ]; then
    exit 1
fi
ldconfig

#-----------------------------------
# PCL
#-----------------------------------

cd ${root_dir}
./build_pcl.sh

if [ $? -ne 0 ]; then
    exit 1
fi

#-----------------------------------
# Ceres
#-----------------------------------

cd ${root_dir}
./build_ceres.sh

if [ $? -ne 0 ]; then
    exit 1
fi

#-----------------------------------
# OpenCV
#-----------------------------------

cd ${thirdparty_dir}
${root_dir}/make_opencv.sh

if [ $? -ne 0 ]; then
    exit 1
fi
ldconfig
