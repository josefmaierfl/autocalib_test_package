FROM conanio/gcc8 as dependencies

ENV LANG=C.UTF-8 LC_ALL=C.UTF-8

USER root
SHELL ["/bin/bash", "-c"]
RUN export DEBIAN_FRONTEND=noninteractive && apt-get update && apt-get install -y software-properties-common && apt-get clean
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
RUN export DEBIAN_FRONTEND=noninteractive && apt-get update && apt-get install -y libboost-all-dev && apt-get clean
RUN export DEBIAN_FRONTEND=noninteractive && apt-get update && apt-get install -y texlive-latex-base texlive-fonts-recommended texlive-fonts-extra texlive-latex-extra && apt-get clean
#RUN export DEBIAN_FRONTEND=noninteractive && add-apt-repository -y ppa:deadsnakes/ppa
#RUN export DEBIAN_FRONTEND=noninteractive && apt-get update && apt-get install -y python3.6 && apt-get clean
#RUN python3 -m pip install --upgrade pip setuptools wheel
RUN export DEBIAN_FRONTEND=noninteractive && apt-get update && apt-get install -y nano && apt-get clean

ADD ci /ci
RUN mkdir /ci/tmp/

COPY miniconda_linux_install.sh /ci/tmp/
COPY build_ngransac.sh /ci/tmp/
COPY py_test_scripts/requirements_necce_no_vers.txt /ci/tmp/
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

#FROM dependencies as usercode
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
#SHELL ["/usr/bin/env", "bash", "--login", "-c"]
#RUN echo 'alias python=python3' >> ~/.bashrc
CMD [ "/bin/bash" ]
