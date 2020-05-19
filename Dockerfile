FROM veriblock/prerelease-btc

RUN apt-get update && apt-get install --no-install-recommends -y libasan4

ADD . /app
WORKDIR /app

RUN export VERIBLOCK_POP_CPP_VERSION=$(awk -F '=' '/\$\(package\)_version/{print $NF}' $PWD/depends/packages/veriblock-pop-cpp.mk | head -n1); \
    (\
     cd /opt; \
     wget https://github.com/VeriBlock/alt-integration-cpp/archive/${VERIBLOCK_POP_CPP_VERSION}.tar.gz; \
     tar -xf ${VERIBLOCK_POP_CPP_VERSION}.tar.gz; \
     cd alt-integration-cpp-${VERIBLOCK_POP_CPP_VERSION}; \
     mkdir build; \
     cd build; \
     cmake .. -DCMAKE_BUILD_TYPE=Debug -DASAN=ON -DTESTING=OFF; \
     make -j2 install \
    )

RUN ./autogen.sh
RUN CC=gcc-7 CXX=g++-7 ./configure \
  --without-gui \
  --disable-tests \
  --disable-bench \
  --disable-ccache \
  --disable-man \
  --with-libs=no \
  --with-sanitizers=address
RUN make -j4 install

WORKDIR /root

# some cleanup to decrease image size
RUN rm -rf /app