#dependencies
#requires thirdparty environment variable THIRDPARTYROOT

#packages
sudo apt-get install cmake build-essential


DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
GCC_VER="$(gcc --version | grep ^gcc | sed 's/^.* //g')"
pushd $DIR
cd ../
mkdir linux
cd linux
rm ./CMakeCache.txt
cmake ../../ -DCMAKE_BUILD_TYPE=Release
make -j8
popd

mkdir "../../lib/linux64gcc${GCC_VER}"
find ../linux -name  \*.a -type f -exec cp {} "../../lib/linux64gcc${GCC_VER}" \;

pushd $DIR
cd ../
rm -Rf linux_debug
mkdir linux_debug
cd linux_debug
rm ./CMakeCache.txt
cmake ../../ -DCMAKE_BUILD_TYPE=Debug
make -j8
popd


find ../linux_debug -name  \*.a -type f -exec  sh -c 'mv ${0} ${0%.a}d.a' {}  \;
find ../linux_debug -name  \*.a -type f -exec cp {} "../../lib/linux64gcc${GCC_VER}" \;
