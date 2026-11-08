# syntax=docker/dockerfile:1
# Reproducible Linux build. Each stage can be built on its own:
#   docker build --target test .     # build + ctest
#   docker build --target lint .     # clang-tidy over all sources
# Wrapper scripts: scripts/docker.sh, scripts/docker.ps1
ARG UBUNTU_VERSION=24.04

FROM ubuntu:${UBUNTU_VERSION} AS toolchain
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update \
    && apt-get install --yes --no-install-recommends build-essential cmake ninja-build clang-tidy \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /src

FROM toolchain AS build
COPY CMakeLists.txt CMakePresets.json .clang-tidy ./
COPY include include
COPY src src
COPY app app
COPY tests tests
COPY scripts scripts
RUN sh scripts/build.sh release

FROM build AS test
RUN ctest --preset release

FROM build AS lint
RUN clang-tidy --warnings-as-errors='*' -p build/release $(find src app tests -name '*.cpp' | sort)
