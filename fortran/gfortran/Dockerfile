# Toolchain image: MinGW-w64 gfortran cross compilers (32 + 64 bit Windows DLLs)
FROM debian:bookworm-slim
RUN apt-get update \
 && apt-get install -y --no-install-recommends \
      gfortran-mingw-w64-i686-win32 gfortran-mingw-w64-x86-64-win32 patch \
 && rm -rf /var/lib/apt/lists/*
WORKDIR /work
