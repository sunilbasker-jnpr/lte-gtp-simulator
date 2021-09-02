FROM ubuntu:20.04

# LABEL LTE-GTP-SIMULATOR
LABEL maintainer="admin@juniper.net"
LABEL version="1.0"
RUN apt update && apt -y upgrade && apt install -y build-essential && apt install -y net-tools
RUN apt install -y gcc
ENV TZ=America/Los_Angeles
ENV DEBIAN_FRONTEND noninteractive
RUN apt install -y git
RUN apt install -y  cmake
RUN apt install -y libncurses5-dev
RUN apt install -y tcpdump iputils-ping vim gdb
RUN git clone https://github.com/ThomasHabets/gtping.git
RUN git clone https://github.com/sunilbasker-jnpr/LTE-GTP-Simulator.git
RUN cd /LTE-GTP-Simulator/ && mkdir build && cd build && cmake .. && make
