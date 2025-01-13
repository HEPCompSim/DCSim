FROM ubuntu:24.04

LABEL org.opencontainers.image.authors="maximilian.horzela@kit.edu"

ENV NCORES=4

USER root
WORKDIR /tmp

RUN echo "wrench ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers
SHELL ["/bin/bash", "-c"]

###########################################################
# Install prerequisites
###########################################################

RUN apt-get update && apt-get upgrade -y && \
    apt-get install -y \
        cmake python3 pip gcc make gfortran libboost-all-dev git \
        python3-pip python3-setuptools python3-wheel && \
    apt-get -y autoclean && apt-get -y autoremove && \
    rm -rf /var/lib/apt-get/lists/*
RUN apt-get install -y \
        python3-numpy python3-matplotlib python3-scipy python3-pandas
# RUN python3 -m pip install --upgrade --no-input --break-system-packages \
#         numpy matplotlib scipy pandas

###########################################################
# Compile and install prerequisite software packages
###########################################################

RUN git clone https://github.com/zeux/pugixml.git && \
    mkdir -p pugixml/build && pushd pugixml/build && \
    git checkout tags/v1.12.1 && \
    cmake .. && make -j${NCORES} && make install && popd && \
    rm -rf pugixml
RUN git clone https://github.com/nlohmann/json.git && \
    mkdir -p json/build && pushd json/build && \
    git checkout tags/v3.11.2 && \
    cmake .. && make -j${NCORES} && make install && popd && \
    rm -rf json
RUN git clone https://github.com/google/googletest.git && \
    mkdir -p googletest/build && pushd googletest/build && \
    git checkout tags/release-1.12.1 && \
    cmake .. && make -j${NCORES} && make install && popd && \
    rm -rf googletest

###########################################################
# Compile and install SimGrid(+Modules) & WRENCH
###########################################################

RUN git clone https://framagit.org/simgrid/simgrid.git && \
    mkdir -p simgrid/build && pushd simgrid/build && \
    git checkout tags/v3.36 && \
    cmake .. && make -j${NCORES} && make install && popd && \
    rm -rf simgrid
RUN git clone https://github.com/simgrid/file-system-module.git && \
    mkdir -p file-system-module/build && pushd file-system-module/build && \
    git checkout tags/v0.2 && \
    cmake .. && make -j${NCORES} && make install && popd && \
    rm -rf file-system-module
RUN git clone https://github.com/wrench-project/wrench.git && \
    mkdir -p wrench/build && pushd wrench/build && \
    git checkout tags/v2.5 && \
    cmake .. && make -j${NCORES} && make install && popd && \
    rm -rf wrench

###########################################################
# Compile and install DCSim
###########################################################

RUN git clone https://github.com/HEPCompSim/DCSim.git && \
    mkdir -p DCSim/build && pushd DCSim/build && \
    git checkout simcal-calibrator && \
    cmake .. && make -j${NCORES} && make install && popd && ldconfig && \
    mkdir -p /home/DCSim/data && cp -r DCSim/data/* /home/DCSim/data && \
    rm -rf DCSim

###########################################################
# Install simcal calibration framework
###########################################################
RUN git clone https://github.com/HerrHorizontal/Grand-Unified-Calibration-Framework.git && \
    pushd Grand-Unified-Calibration-Framework && \
    # python3 -m pip install -r requirements.txt && \
    python3 -m pip install --break-system-packages . && popd && \
    rm -rf Grand-Unified-Calibration-Framework

###########################################################
# Set user
###########################################################

#USER wrench
WORKDIR /home/dcsim

# set user's environment variable
ENV CXX="g++" CC="gcc"
ENV LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/usr/local/lib
