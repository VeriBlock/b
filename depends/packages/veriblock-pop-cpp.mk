package=veriblock-pop-cpp
$(package)_version=69f2fff3f7f542f3ed86322090d0315d08ca6aa5
$(package)_download_path=https://github.com/VeriBlock/alt-integration-cpp/archive/
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=7984ebee97b02167e7bc57c59fee7df3fbc5e12694c7df6782158e3d58a6c7cf
$(package)_build_subdir=build

define $(package)_preprocess_cmds
  mkdir -p build
endef

ifeq ($(strip $(HOST)),)
  define $(package)_config_cmds
    cmake -DCMAKE_INSTALL_PREFIX=$(host_prefix) -DCMAKE_BUILD_TYPE=Release \
    -DTESTING=OFF -DWITH_ROCKSDB=OFF -DSHARED=OFF ..
  endef
else ifeq ($(HOST), x86_64-apple-darwin16)
  define $(package)_config_cmds
    cmake -DCMAKE_C_COMPILER=$(darwin_CC) -DCMAKE_CXX_COMPILER=$(darwin_CXX) \
    -DCMAKE_INSTALL_PREFIX=$(host_prefix) -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_ARCHITECTURES=x86_64 -DCMAKE_SYSTEM_NAME=Darwin -DCMAKE_SYSTEM_PROCESSOR=x86_64 \
    -DCMAKE_C_COMPILER_TARGET=$(HOST) -DCMAKE_CXX_COMPILER_TARGET=$(HOST) \
    -DCMAKE_OSX_SYSROOT=$(OSX_SDK) -DTESTING=OFF -DWITH_ROCKSDB=OFF \
    -DSHARED=OFF ..
  endef
else ifeq ($(HOST), x86_64-pc-linux-gnu)
  define $(package)_config_cmds
    cmake -DCMAKE_INSTALL_PREFIX=$(host_prefix) -DCMAKE_BUILD_TYPE=Release \
    -DTESTING=OFF -DWITH_ROCKSDB=OFF -DSHARED=OFF ..
  endef
else
  define $(package)_config_cmds
    cmake -DCMAKE_C_COMPILER=$(HOST)-gcc -DCMAKE_CXX_COMPILER=$(HOST)-g++ \
    -DCMAKE_INSTALL_PREFIX=$(host_prefix) -DTESTING=OFF -DWITH_ROCKSDB=OFF -DSHARED=OFF ..
  endef
endif

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef
