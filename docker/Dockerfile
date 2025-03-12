FROM amazonlinux:2016.09.0.20161028
LABEL maintainer="Jeremy Zheng"

RUN yum update -y
RUN yum upgrade -y
RUN yum groupinstall -y 'Development Tools'
RUN yum install -y which git wget vim \
    python38 python38-devel perl-IPC-Cmd \
    gcc72 gcc72-c++ ninja-build
# boost-devel postgresql96-devel mysql-devel sqlite-devel 
RUN yum clean all

RUN localedef -i en_US -c -f UTF-8 en_US.UTF-8

ENV LC_ALL=en_US.utf-8
ENV LANG=en_US.utf-8

# https://packaging.python.org/en/latest/tutorials/installing-packages/#ensure-you-can-run-pip-from-the-command-line
RUN wget -q -P $HOME/downloads https://bootstrap.pypa.io/get-pip.py
RUN python3 $HOME/downloads/get-pip.py --user

RUN curl -fsSL https://xmake.io/shget.text | bash
RUN echo -e "\nexport XMAKE_ROOT=y" >> $HOME/.profile

RUN bash -lc "pip install --user cmake"

VOLUME /workspace
WORKDIR /workspace

CMD ["/bin/bash", "-l"]
