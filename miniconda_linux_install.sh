#!/usr/bin/env bash
set -e

CURR_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Name of application to install
AppName="NGRANSAC"

# Set your project's install directory name here
InstallDir="miniconda3"

# Dependencies installed by Conda
# Comment out the next line if no Conda dependencies
#CondaDeps="numpy scipy scikit-learn pandas"
#CondaDepsFile="requirements_necce_no_vers.txt"
CondaDepsFileConda="requirements_conda_install.txt"
CondaDepsFilePip="requirements_pip.txt"

# Install the package from PyPi
# Comment out next line if installing locally
# PyPiPackage="mypackage"

# Local packages to install
# Useful if your application is not in PyPi
# Distribute this with a .tar.gz and use this variable
# Comment out the next line if no local package to install
# LocalPackage="mypackage.tar.gz"

# Entry points to add to the path
# Comment out the next line of no entry point
#   (Though not sure why this script would be useful otherwise)
EntryPoint="NGRANSAC"

echo
echo "Installing $AppName"

FullInstallDir="$(pwd)/$InstallDir"
echo
echo "Installing into: $FullInstallDir"
echo

# Miniconda doesn't work for directory structures with spaces
if [[ $(pwd) == *" "* ]]
then
    echo "ERROR: Cannot install into a directory with a space in its path" >&2
    echo "Exiting..."
    echo
    exit 1
fi

# Test if new directory is empty.  Exit if it's not
if [ -d $FullInstallDir ]; then
    if [ "$(ls -A $FullInstallDir)" ]; then
        echo "ERROR: Directory is not empty" >&2
        echo "If you want to install into $FullInstallDir, "
        echo "clear the directory first and run this script again."
        echo "Exiting..."
        echo
        exit 1
    fi
fi

# Download and install Miniconda
set +e
curl "https://repo.continuum.io/miniconda/Miniconda3-latest-Linux-x86_64.sh" -o Miniconda_Install.sh
if [ $? -ne 0 ]; then
    curl "http://repo.continuum.io/miniconda/Miniconda3-latest-Linux-x86_64.sh" -o Miniconda_Install.sh
fi
set -e

bash Miniconda_Install.sh -b -f -p $FullInstallDir

# Activate the new environment
PATH="$FullInstallDir/bin":$PATH

# Make the new python environment completely independent
# Modify the site.py file so that USER_SITE is not imported
# python -s << END
# import site
# site_file = site.__file__.replace(".pyc", ".py");
# with open(site_file) as fin:
#     lines = fin.readlines();
# for i,line in enumerate(lines):
#     if(line.find("ENABLE_USER_SITE = None") > -1):
#         user_site_line = i;
#         break;
# lines[user_site_line] = "ENABLE_USER_SITE = False\n"
# with open(site_file,'w') as fout:
#     fout.writelines(lines)
# END

conda update -n base -c defaults conda
#source ~/.bashrc
#PATH="$(pwd)/$InstallDir/bin":$PATH
eval "$(conda shell.bash hook)"
conda init
conda create -n $AppName python=3.6
PATH="$FullInstallDir/bin":$PATH
conda activate $AppName

# Install Conda Dependencies
conda install pip -y
if [[ $CondaDeps ]]; then
    conda install $CondaDeps -y
fi
if [[ $CondaDepsFile ]]; then
    while read requirement; do conda install --yes -c defaults -c conda-forge -c anaconda $requirement || sudo pip install $requirement; done < "${CURR_DIR}/$CondaDepsFile"
    conda uninstall --yes libtiff
fi
if [[ $CondaDepsFileConda ]]; then
    conda install --yes -c defaults -c conda-forge -c anaconda --file "${CURR_DIR}/$CondaDepsFileConda"
    # conda install pytorch torchvision cudatoolkit=10.1 -c pytorch
    conda install -c pytorch magma-cuda102
    conda uninstall --yes libtiff
fi
if [[ $CondaDepsFilePip ]]; then
    while read requirement; do sudo pip install $requirement; done < "${CURR_DIR}/$CondaDepsFilePip"
fi

# Install Package from PyPi
if [[ $PyPiPackage ]]; then
    pip install $PyPiPackage -q
fi

# Install Local Package
if [[ $LocalPackage ]]; then
    pip install $LocalPackage -q
fi

# Cleanup
rm Miniconda_Install.sh
#conda clean -itp --yes

conda update mkl

echo "export PATH=$FullInstallDir/bin:$PATH" >> ~/.bashrc
sudo ln -s $FullInstallDir/etc/profile.d/conda.sh /etc/profile.d/conda.sh
echo ". $FullInstallDir/etc/profile.d/conda.sh" >> ~/.bashrc
echo "conda activate $AppName" >> ~/.bashrc

# Add Entry Point to the path
# if [[ $EntryPoint ]]; then
#
#     cd $InstallDir
#     mkdir Scripts
#     ln -s ../bin/$EntryPoint Scripts/$EntryPoint
#     sudo ln -s ../etc/profile.d/conda.sh /etc/profile.d/conda.sh
#
#     echo "$EntryPoint script installed to $(pwd)/Scripts"
#     echo
#     # echo "Add folder to path by appending to .bashrc?"
#     # read -p "[y/n] >>> " -r
#     # echo
#     # if [[ ($REPLY == "yes") || ($REPLY == "Yes") || ($REPLY == "YES") ||
#     #     ($REPLY == "y") || ($REPLY == "Y")]]
#     # then
#         echo "export PATH=\"$(pwd)/Scripts\":\$PATH" >> ~/.bashrc
#         echo "conda activate $AppName" >> ~/.bashrc
#         echo "Your PATH was updated."
#         echo "Restart the terminal for the change to take effect"
#     # else
#     #     echo "Your PATH was not modified."
#     # fi
#
#     cd ..
# fi

echo
echo "$AppName Install Successfully"
