FROM ubuntu:latest

MAINTAINER Maximilian Horzela <maximilian.horzela@kit.edu>

ENV NCORES=4

USER root
WORKDIR /tmp

RUN echo "wrench ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers
SHELL ["/bin/bash", "-c"]

###########################################################
# Install prerequisites
###########################################################

RUN apt-get update && apt-get upgrade -y
RUN apt-get install -y cmake python3 pip gcc make gfortran libboost-all-dev git
RUN python3 -m pip install pip setuptools numpy matplotlib scipy pandas --upgrade --no-input

###########################################################
# Compile and install prerequisite software packages
###########################################################

RUN git clone https://github.com/zeux/pugixml.git && mkdir -p pugixml/build && pushd pugixml/build && git checkout tags/v1.12.1 && cmake .. && make -j${NCORES} && make install && popd
RUN rm -rf pugixml
RUN git clone https://github.com/nlohmann/json.git && mkdir -p json/build && pushd json/build && git checkout tags/v3.11.2 && cmake .. && make -j${NCORES} && make install && popd
RUN rm -rf json
RUN git clone https://github.com/google/googletest.git && mkdir -p googletest/build && pushd googletest/build && git checkout tags/release-1.12.1 && cmake .. && make -j${NCORES} && make install && popd
RUN rm -rf googletest

###########################################################
# Compile and install SimGrid & WRENCH
###########################################################

RUN git clone https://framagit.org/simgrid/simgrid.git && mkdir -p simgrid/build && pushd simgrid/build && cmake .. && make -j${NCORES} && make install && popd
RUN rm -rf simgrid
RUN git clone https://github.com/wrench-project/wrench.git && mkdir -p wrench/build && pushd wrench/build && cmake .. && make -j${NCORES} && make install && popd
RUN rm -rf wrench

###########################################################
# Compile and install DCSim
###########################################################

RUN git clone https://github.com/HEPCompSim/DCSim.git && mkdir -p DCSim/build && pushd DCSim/build && cmake .. && make -j${NCORES} && make install && popd
RUN ldconfig

###########################################################
# Set user
###########################################################

USER wrench
WORKDIR /home/wrench

# set user's environment variable
ENV CXX="g++-11" CC="gcc-11"
ENV LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/usr/local/lib
