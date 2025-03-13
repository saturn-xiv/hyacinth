FROM amazonlinux:2016.09.0.20161028
LABEL maintainer="Jeremy Zheng"

RUN yum update -y
RUN yum upgrade -y
RUN yum groupinstall -y 'Development Tools'
RUN yum install -y which git wget vim \
    python38 python38-devel perl-IPC-Cmd \
    gcc72 gcc72-c++
# 
# boost-devel postgresql96-devel mysql-devel sqlite-devel 
RUN yum clean all

RUN localedef -i en_US -c -f UTF-8 en_US.UTF-8

ENV LC_ALL=en_US.utf-8
ENV LANG=en_US.utf-8

RUN wget -q -P $HOME/downloads https://github.com/ninja-build/ninja/releases/download/v1.12.1/ninja-linux.zip
RUN unzip $HOME/downloads/ninja-linux.zip -d $HOME/build
RUN mv $HOME/build/ninja /usr/local/bin/

# https://packaging.python.org/en/latest/tutorials/installing-packages/#ensure-you-can-run-pip-from-the-command-line
RUN wget -q -P $HOME/downloads https://bootstrap.pypa.io/get-pip.py
RUN python3 $HOME/downloads/get-pip.py --user
RUN echo 'export PATH=$HOME/.local/bin:$PATH' >> $HOME/.profile

# https://learn.microsoft.com/en-us/vcpkg/get_started/get-started?pivots=shell-bash
RUN git clone -b "2025.02.14" https://github.com/microsoft/vcpkg.git $HOME/local/vcpkg
RUN echo 'export VCPKG_ROOT=$HOME/local/vcpkg' >> $HOME/.profile
RUN echo 'export VCPKG_DISABLE_METRICS=1' >> $HOME/.profile
RUN $HOME/local/vcpkg/bootstrap-vcpkg.sh

RUN bash -lc "pip install --user cmake"

VOLUME /workspace
WORKDIR /workspace

CMD ["/bin/bash", "-l"]
