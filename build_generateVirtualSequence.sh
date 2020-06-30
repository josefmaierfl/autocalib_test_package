#!/bin/bash

root_dir=`pwd`
virtsequ_dir=${root_dir}/generateVirtualSequence
build_dir=${virtsequ_dir}/build

# Build project generateVirtualSequence
mkdir ${build_dir}
cd ${build_dir}
cmake ../ -DCMAKE_BUILD_TYPE=Release
make -j "$(nproc)"
if [ $? -ne 0 ]; then
    exit 1
fi

copy_dir0=${root_dir}/tmp/generateVirtualSequence
copy_dir=${copy_dir0}/build
mkdir -p ${copy_dir}
find ${build_dir} -type f \( -executable -o -name \*.so -name \*.py \) -exec cp {} ${copy_dir} \;
find ${virtsequ_dir} -type f \( -name \*.py \) -exec cp {} ${copy_dir0} \;
