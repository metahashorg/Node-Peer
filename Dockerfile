FROM ubuntu:18.04 AS builder
MAINTAINER oleg@romanenko.ro

RUN apt-get -q -y update && apt-get -q -y install apt-utils \
    && apt-get -q -y install gnupg gcc-8 g++-8 build-essential git libre2-dev libunwind-dev wget git autoconf \
                             libltdl-dev flex bison texinfo

# GCC
RUN   update-alternatives --quiet --remove-all gcc \
    ; update-alternatives --quiet --remove-all g++ \
    ; update-alternatives --quiet --remove-all cc \
    ; update-alternatives --quiet --remove-all cpp \
    ; update-alternatives --quiet --install /usr/bin/gcc gcc /usr/bin/gcc-8 20 \
    ; update-alternatives --quiet --install /usr/bin/cc cc /usr/bin/gcc-8 20 \
    ; update-alternatives --quiet --install /usr/bin/g++ g++ /usr/bin/g++-8 20 \
    ; update-alternatives --quiet --install /usr/bin/cpp cpp /usr/bin/g++-8 20 \
    ; update-alternatives --quiet --config gcc \
    ; update-alternatives --quiet --config cc \
    ; update-alternatives --quiet --config g++ \
    ; update-alternatives --quiet --config cpp

# cmake
ARG CMAKE_VER=v3.14.1
RUN git clone https://github.com/Kitware/CMake.git /tmp/cmake && cd /tmp/cmake &&  git checkout ${CMAKE_VER} \
    && ./bootstrap --parallel=`nproc` && make --jobs=`nproc` && make install

# libev
ARG LIBEV_VER=4.25
RUN wget http://dist.schmorp.de/libev/Attic/libev-${LIBEV_VER}.tar.gz -O /tmp/libev.tar.gz \
    && tar xzf /tmp/libev.tar.gz -C /tmp && cd /tmp/libev-${LIBEV_VER} && ./configure && make --jobs=`nproc` \
    && make install

# libfmt
ARG LIBFMT_VER=5.3.0
RUN git clone https://github.com/fmtlib/fmt.git /tmp/libfmt && cd /tmp/libfmt && git checkout ${LIBFMT_VER} \
    && cmake -DCMAKE_BUILD_TYPE=Release -DFMT_TEST=OFF -DFMT_DOC=OFF -DFMT_INSTALL=ON -DFMT_USE_CPP11=ON -DFMT_USE_CPP14=ON -DCMAKE_CXX_STANDARD=17 . \
    && make --jobs=`nproc` && make install

# libgoogle-perftools
ARG LIBPERFTOOLS_VER=gperftools-2.7
RUN git clone https://github.com/gperftools/gperftools.git /tmp/gperftools && cd /tmp/gperftools \
    && git checkout ${LIBPERFTOOLS_VER} && ./autogen.sh && ./configure --with-tcmalloc-pagesize=64 \
    && make --jobs=`nproc` && make install

# openssl
ARG LIBOPENSSL_VER=OpenSSL_1_1_1b
RUN git clone https://github.com/openssl/openssl.git /tmp/openssl && cd /tmp/openssl &&  git checkout ${LIBOPENSSL_VER} \
    && ./config --libdir=lib --openssldir=ssl -Wl,-rpath,/usr/local/lib && make --jobs=`nproc` && make install

# boost
ARG LIBBOOST_VER=1.70.0
RUN wget https://boostorg.jfrog.io/artifactory/main/release/1.70.0/source/boost_1_70_0.tar.bz2 -O /tmp/boost.tar.bz2 \
    && tar xjf /tmp/boost.tar.bz2 -C /tmp && cd /tmp/boost_1_70_0 && ./bootstrap.sh --without-icu \
    && ./b2 -j`nproc` threading=multi link=static,shared variant=release runtime-link=shared && ./b2 install

# libconfig++
ARG LIBCONFIG_VER=v1.7.2
RUN git clone https://github.com/hyperrealm/libconfig.git /tmp/libconfig && cd /tmp/libconfig \
    && git checkout ${LIBCONFIG_VER} && autoreconf && ./configure && make --jobs=`nproc` \
    && make --jobs=`nproc` && make install

# libsecp256k1
ARG LIBSECP_VER=efad3506a8937162e8010f5839fdf3771dfcf516
RUN git clone https://github.com/bitcoin-core/secp256k1.git /tmp/libsecp256k1 && cd /tmp/libsecp256k1 && git checkout ${LIBSECP_VER} \
    && ./autogen.sh && ./configure --enable-static --enable-openssl-tests=no --enable-tests=no --enable-exhaustive-tests=no --enable-coverage=no --enable-benchmark=no \
    && make --jobs=`nproc` && make install

RUN ldconfig


COPY . /builder
RUN cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -S /builder -B /builder/app
RUN make --jobs=`nproc` -C /builder/app
CMD ["/bin/bash"]


FROM ubuntu:18.04
MAINTAINER oleg@romanenko.ro

WORKDIR /conf
ENTRYPOINT ["/app/peer_node"]
CMD [""]

COPY --from=builder /usr/local/lib/libtcmalloc.so.4 /usr/lib/x86_64-linux-gnu/libunwind.so.8 /usr/lib/
RUN ldconfig

COPY --from=builder /builder/app/bin/peer_node /app/
