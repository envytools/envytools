FROM ubuntu:xenial
RUN apt-get update && apt-get -y install \
    clang \
    cmake \
    libxml2-dev \
    flex \
    bison \
    pkg-config \
    # Optional dependency needed by hwtest \
    libpciaccess-dev \
    # Optional dependency needed by demmt \
    libdrm-dev \
    libseccomp-dev \
    # Optional dependency needed by nva \
    libpciaccess-dev \
    libx11-dev \
    libxext-dev \
    # Optional dependency needed by nvapy
    python3-dev \
    cython3 \
    # Optional dependencies needed by vdpow \
    libpciaccess-dev \
    libx11-dev \
    libvdpau-dev
	
RUN mkdir /build
WORKDIR /build
