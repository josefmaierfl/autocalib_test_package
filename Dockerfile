FROM conanio/gcc8 as dependencies

ENV LANG=C.UTF-8 LC_ALL=C.UTF-8

USER root
SHELL ["/bin/bash", "-c"]
RUN export DEBIAN_FRONTEND=noninteractive && apt-get update && apt-get install -y software-properties-common apt-utils && apt-get clean
RUN export DEBIAN_FRONTEND=noninteractive && apt-get update && add-apt-repository -y 'deb http://security.ubuntu.com/ubuntu xenial-security main'
RUN export DEBIAN_FRONTEND=noninteractive && apt-get update && apt-get install -y build-essential cmake pkg-config && apt-get clean
RUN export DEBIAN_FRONTEND=noninteractive && apt-get update && apt-get install -y wget \
	libtbb2 \
	libtbb-dev \
	libglew-dev \
	qt5-default \
	libxkbcommon-dev \
	libflann-dev \
	libpng-dev \
	libgtk-3-dev \
	libgtkglext1 \
	libgtkglext1-dev \
	libtiff-dev \
	libtiff5-dev \
	libtiffxx5 \
	libjpeg-dev \
	libjasper1 \
	libjasper-dev \
	libavcodec-dev \
	libavformat-dev \
	libswscale-dev \
	libv4l-dev \
	libxvidcore-dev \
	libx264-dev \
	libdc1394-22-dev \
	openexr \
	libatlas-base-dev \
	gfortran && apt-get clean
RUN export DEBIAN_FRONTEND=noninteractive && apt-get update && apt-get install -y python3-dev && apt-get clean
RUN export DEBIAN_FRONTEND=noninteractive && apt-get update && apt-get install -y libvtk7-dev && apt-get clean
#RUN export DEBIAN_FRONTEND=noninteractive && apt-get update && apt-get install -y libboost-all-dev && apt-get clean
RUN export DEBIAN_FRONTEND=noninteractive && apt-get update && apt-get install -y texlive-latex-base texlive-fonts-recommended texlive-fonts-extra texlive-latex-extra && apt-get clean
#RUN export DEBIAN_FRONTEND=noninteractive && add-apt-repository -y ppa:deadsnakes/ppa
#RUN export DEBIAN_FRONTEND=noninteractive && apt-get update && apt-get install -y python3.6 && apt-get clean
#RUN python3 -m pip install --upgrade pip setuptools wheel
RUN export DEBIAN_FRONTEND=noninteractive && apt-get update && apt-get install -y nano && apt-get clean
RUN export DEBIAN_FRONTEND=noninteractive && apt-get update && apt-get install -y libomp-dev && apt-get clean

#Install CUDA
RUN export DEBIAN_FRONTEND=noninteractive && apt-get update && apt-get install -y --no-install-recommends \
	gnupg2 curl ca-certificates && \
	curl -fsSL https://developer.download.nvidia.com/compute/cuda/repos/ubuntu1804/x86_64/7fa2af80.pub | apt-key add - && \
	echo "deb https://developer.download.nvidia.com/compute/cuda/repos/ubuntu1804/x86_64 /" > /etc/apt/sources.list.d/cuda.list && \
	echo "deb https://developer.download.nvidia.com/compute/machine-learning/repos/ubuntu1804/x86_64 /" > /etc/apt/sources.list.d/nvidia-ml.list && \
	apt-get clean

ENV CUDA_VERSION 10.2.89
ENV CUDA_PKG_VERSION 10-2=$CUDA_VERSION-1

# For libraries in the cuda-compat-* package: https://docs.nvidia.com/cuda/eula/index.html#attachment-a
RUN export DEBIAN_FRONTEND=noninteractive && apt-get update && apt-get install -y --no-install-recommends \
	cuda-cudart-$CUDA_PKG_VERSION \
	cuda-compat-10-2 && \
	ln -s cuda-10.2 /usr/local/cuda && \
  apt-get clean

# Required for nvidia-docker v1
RUN echo "/usr/local/nvidia/lib" >> /etc/ld.so.conf.d/nvidia.conf && \
	echo "/usr/local/nvidia/lib64" >> /etc/ld.so.conf.d/nvidia.conf

ENV PATH /usr/local/nvidia/bin:/usr/local/cuda/bin:${PATH}
ENV LD_LIBRARY_PATH /usr/local/nvidia/lib:/usr/local/nvidia/lib64

# nvidia-container-runtime
ENV NVIDIA_VISIBLE_DEVICES all
ENV NVIDIA_DRIVER_CAPABILITIES compute,utility
ENV NVIDIA_REQUIRE_CUDA "cuda>=10.2 brand=tesla,driver>=384,driver<385 brand=tesla,driver>=396,driver<397 brand=tesla,driver>=410,driver<411 brand=tesla,driver>=418,driver<419"

# cuda runtime
ENV NCCL_VERSION 2.5.6
RUN export DEBIAN_FRONTEND=noninteractive && apt-get update && apt-get install -y --no-install-recommends \
	cuda-libraries-$CUDA_PKG_VERSION \
	cuda-nvtx-$CUDA_PKG_VERSION \
	libcublas10=10.2.2.89-1 \
	libnccl2=$NCCL_VERSION-1+cuda10.2 && \
  apt-mark hold libnccl2 && \
  apt-get clean

# CUDA development
RUN export DEBIAN_FRONTEND=noninteractive && apt-get update && apt-get install -y --no-install-recommends \
	cuda-nvml-dev-$CUDA_PKG_VERSION \
	cuda-command-line-tools-$CUDA_PKG_VERSION \
	cuda-libraries-dev-$CUDA_PKG_VERSION \
  cuda-minimal-build-$CUDA_PKG_VERSION \
  libnccl-dev=$NCCL_VERSION-1+cuda10.2 && \
	apt-get clean

ENV LIBRARY_PATH /usr/local/cuda/lib64/stubs

# cudnn7 libs
ENV CUDNN_VERSION 7.6.5.32
RUN export DEBIAN_FRONTEND=noninteractive && apt-get update && apt-get install -y --no-install-recommends \
	libcudnn7=$CUDNN_VERSION-1+cuda10.2 \
	libcudnn7-dev=$CUDNN_VERSION-1+cuda10.2 && \
  apt-mark hold libcudnn7 && \
  apt-get clean

ADD ci /ci
RUN mkdir /ci/tmp/

COPY miniconda_linux_install.sh /ci/tmp/
COPY build_ngransac.sh /ci/tmp/
COPY py_test_scripts/requirements_necce_no_vers.txt /ci/tmp/
COPY py_test_scripts/requirements_conda_install.txt /ci/tmp/
COPY py_test_scripts/requirements_pip.txt /ci/tmp/
COPY py_test_scripts/requirements_reinstall.txt /ci/tmp/
COPY conda_package_reinstall.sh /ci/tmp/

#RUN export DEBIAN_FRONTEND=noninteractive && set -eux && \
#	apt-get-get update && \
#	apt-get install -y gosu && \
#	rm -rf /var/lib/apt/lists/* && \
# verify that the binary works
#	gosu nobody true #&& chmod +s gosu

USER conan
SHELL ["/usr/bin/env", "bash", "--login", "-c"]
ENV PATH /home/conan/miniconda3/bin:$PATH
RUN /ci/tmp/miniconda_linux_install.sh

USER root
SHELL ["/bin/bash", "--login", "-c"]
RUN conda init bash
RUN conda activate NGRANSAC

RUN cd /ci && ./build_thirdparty.sh
RUN cd /ci && ./copy_thirdparty.sh
RUN cd /ci && ./build_pytorch.sh

FROM dependencies as usercode
COPY generateVirtualSequence /ci/tmp/generateVirtualSequence/
COPY build_generateVirtualSequence.sh /ci/tmp/
RUN cd /ci/tmp && ./build_generateVirtualSequence.sh

COPY matchinglib_poselib /ci/tmp/matchinglib_poselib/
COPY build_matchinglib_poselib.sh /ci/tmp/
RUN cd /ci/tmp && ./build_ngransac.sh
#RUN cd /usr/lib/x86_64-linux-gnu/ && ln -s libboost_python-py3x.so libboost_python3.so
RUN cd /ci/tmp && ./build_matchinglib_poselib.sh

RUN cd /ci/tmp && ./conda_package_reinstall.sh

WORKDIR /app
RUN cp -r /ci/tmp/thirdparty /app/
RUN cp -r /ci/tmp/tmp/. /app/
COPY start_testing.sh /app/
#RUN rm -r /ci

RUN chown -R conan /app

USER conan
RUN echo "export MKL_THREADING_LAYER=TBB" >> ~/.bashrc
#SHELL ["/usr/bin/env", "bash", "--login", "-c"]
#RUN echo 'alias python=python3' >> ~/.bashrc
CMD [ "/bin/bash" ]
