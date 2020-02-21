FROM conanio/gcc8 as dependencies

ENV LANG=C.UTF-8 LC_ALL=C.UTF-8

USER root
RUN export DEBIAN_FRONTEND=noninteractive && apt update && apt install -y wget libtbb-dev libglew-dev qt5-default libxkbcommon-dev libflann-dev libpng-dev libgtkglext1-dev && apt clean
RUN export DEBIAN_FRONTEND=noninteractive && apt update && apt install -y libvtk7-dev && apt clean
RUN export DEBIAN_FRONTEND=noninteractive && apt update && apt install -y libboost-all-dev && apt clean
RUN export DEBIAN_FRONTEND=noninteractive && apt update && apt install -y texlive-latex-base texlive-fonts-recommended texlive-fonts-extra texlive-latex-extra && apt clean
RUN export DEBIAN_FRONTEND=noninteractive && apt update && apt install -y software-properties-common && apt clean
RUN export DEBIAN_FRONTEND=noninteractive && add-apt-repository -y ppa:deadsnakes/ppa
RUN export DEBIAN_FRONTEND=noninteractive && apt update && apt install -y python3.7 && apt clean
RUN export DEBIAN_FRONTEND=noninteractive && apt update && apt install -y nano && apt clean

ADD ci /ci
RUN cd /ci && ./build_thirdparty.sh
RUN cd /ci && ./copy_thirdparty.sh
RUN python3 -m pip install --upgrade pip setuptools wheel
#ADD py_test_scripts/requirements.txt .
#RUN python3 -m pip install -r requirements.txt && rm requirements.txt
#RUN python3 -m pip list --outdated --format=freeze | grep -v '^\-e' | cut -d = -f 1  | xargs -n1 python3 -m pip install -U
#RUN python3 -m pip install pytorch==1.2.0
COPY miniconda_linux_install.sh /ci/tmp/
COPY build_ngransac.sh /ci/tmp/
COPY py_test_scripts/requirements_part_no_vers.txt /ci/tmp/

RUN export DEBIAN_FRONTEND=noninteractive && set -eux && \
	apt-get update && \
	apt-get install -y gosu && \
	rm -rf /var/lib/apt/lists/* && \
# verify that the binary works
	gosu nobody true #&& chmod +s gosu

USER conan
SHELL ["/usr/bin/env", "bash", "--login", "-c"]
ENV PATH /home/conan/miniconda3/bin:$PATH
RUN /ci/tmp/miniconda_linux_install.sh

FROM dependencies as usercode
USER root
SHELL ["/bin/bash", "--login", "-c"]
COPY generateVirtualSequence /ci/tmp/generateVirtualSequence/
COPY build_generateVirtualSequence.sh /ci/tmp/
RUN cd /ci/tmp && ./build_generateVirtualSequence.sh

COPY matchinglib_poselib /ci/tmp/matchinglib_poselib/
COPY build_matchinglib_poselib.sh /ci/tmp/
#USER conan
#SHELL ["/usr/bin/env", "bash", "--login", "-c"]
RUN conda init bash
RUN conda activate NGRANSAC
RUN cd /ci/tmp && ./build_ngransac.sh
RUN cd /ci/tmp && ./build_matchinglib_poselib.sh

USER root
SHELL ["/bin/bash", "--login", "-c"]
WORKDIR /app
RUN cp -r /ci/tmp/thirdparty /app/
RUN cp -r /ci/tmp/tmp/. /app/
COPY start_testing.sh /app/
#RUN rm -r /ci

RUN chown -R conan /app

USER conan
SHELL ["/usr/bin/env", "bash", "--login", "-c"]
#RUN echo 'alias python=python3' >> ~/.bashrc
CMD [ "/bin/bash" ]
