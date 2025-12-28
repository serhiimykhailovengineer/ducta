FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Europe/Kyiv

RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    ninja-build \
    cmake \
    git \
    gdb \
    lcov \
    valgrind \
    python3 \
    python3-pip \
    python3-venv \
    python-is-python3 \
    ca-certificates \
    curl \
 && python3 -m pip install --no-cache-dir --upgrade pip setuptools wheel \
 && python3 -m pip install --no-cache-dir --upgrade conan \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace
