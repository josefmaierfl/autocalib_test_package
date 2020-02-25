#!/bin/bash

root_dir=`pwd`
thirdparty_dir=${root_dir}/thirdparty

eval "$(conda shell.bash hook)"
conda activate NGRANSAC

cd ${thirdparty_dir}
Boost_VERSION=1.72.0
Boost_ROOT="boost_$(echo ${Boost_VERSION} | tr . _)"
Boost_FILEN="${Boost_ROOT}.tar.bz2"
wget -q https://dl.bintray.com/boostorg/release/${Boost_VERSION}/source/${Boost_FILEN}
tar --bzip2 -xf ${Boost_FILEN}
rm -rf ${Boost_FILEN}

cd ${Boost_ROOT}
./bootstrap.sh --with-python="$(which python)" \
  --with-python-root="$(python -c "from distutils.sysconfig import get_config_var; print(get_config_var('LIBDEST'))")" \
  --with-python-version="$(python -c "from distutils.sysconfig import get_python_version; print(get_python_version())")"
./b2 install
