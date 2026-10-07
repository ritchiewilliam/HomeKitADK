# Build stage
FROM ubuntu:18.04

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Etc/UTC

RUN echo ========== Install dependencies ========== \
  && apt-get update && apt-get install -y \
    clang \
    gdb \
    git \
    libavahi-compat-libdnssd-dev \
    libssl-dev \
    make \
    openssh-server \
    perl \
    unzip \
    wget \
    libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libgstreamer-plugins-bad1.0-dev gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav gstreamer1.0-tools gstreamer1.0-x gstreamer1.0-alsa gstreamer1.0-gl gstreamer1.0-gtk3 gstreamer1.0-qt5 gstreamer1.0-pulseaudio \
  && apt-get clean \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /build
