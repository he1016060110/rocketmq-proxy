FROM centos:7

WORKDIR /root/

RUN buildDeps="gcc-c++ automake autoconf libtool bzip2-devel wget tar unzip make zlib-devel strace telnet gdb vim openssl-devel openssl git libcurl-devel " \
    && yum update -y \
    && yum install -y $buildDeps \
    && yum clean all -y \
    && yum autoremove -y

RUN wget https://github.com/Kitware/CMake/releases/download/v3.16.4/cmake-3.16.4-Linux-x86_64.tar.gz \
    && tar -zxvf cmake-3.16.4-Linux-x86_64.tar.gz && rm cmake-3.16.4-Linux-x86_64.tar.gz \
    && mv cmake-3.16.4-Linux-x86_64 /usr/local/cmake-3.16.4

ENV PATH=/usr/local/cmake-3.16.4/bin:$PATH

RUN wget https://github.com/he1016060110/rocketmq-client-cpp/archive/xhj-2.0.1.tar.gz && tar -zxvf xhj-2.0.1.tar.gz \
    && rm -rf xhj-2.0.1.tar.gz \
    && cd rocketmq-client-cpp-xhj-2.0.1 \
    && sh build.sh && mkdir /root/rocketmq-client-cpp-xhj-2.0.1/doc && cd /root/rocketmq-client-cpp-xhj-2.0.1/tmp_build_dir && make install && cd ../tmp_down_dir/boost_1_58_0/ \
    && ./b2 -j8 cflags=-fPIC cxxflags=-fPIC --with-atomic --with-thread --with-system --with-chrono --with-date_time \
    --with-log --with-regex --with-serialization --with-filesystem --with-coroutine --with-locale --with-iostreams threading=multi \
    link=static release install --prefix=/usr \
    && rm -rf /root/rocketmq-client-cpp-xhj-2.0.1

ENV PKG_CONFIG_PATH=/usr/local/lib/pkgconfig
ENV LD_LIBRARY_PATH=/usr/local/lib

RUN git clone -b v1.25.0 https://github.com/grpc/grpc \
    && cd grpc \
    && git submodule update --init \
    && mkdir -p cmake/build \
    && cd cmake/build \
    && cmake ../.. \
    && make -j4 && make install

RUN cd grpc/third_party/protobuf

#RUN git clone https://github.com/he1016060110/rocketmq-proxy.git && cd rocketmq-proxy && cmake . && make -j4 \
#    && make install && rm -rf /root/rocketmq-proxy
