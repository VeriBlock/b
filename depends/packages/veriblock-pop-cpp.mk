package=veriblock-pop-cpp
$(package)_version=6261b6e62e8946a39aade4d179449c6875c20983
$(package)_download_path=https://github.com/VeriBlock/alt-integration-cpp/archive/
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=1c372b6480f38b1f56c3d00c3585f8f5d084d83c17dfcd8f72f26403119b3132
$(package)_build_subdir=build
$(package)_build_type=$(BUILD_TYPE)
$(package)_asan=$(ASAN)

define $(package)_preprocess_cmds
  mkdir -p build
endef

ifeq ($(strip $(HOST)),)
  define $(package)_config_cmds
    cmake -DCMAKE_INSTALL_PREFIX=$(host_prefix) -DCMAKE_BUILD_TYPE=$(package)_build_type \
    -DTESTING=OFF -DSHARED=OFF -DASAN:BOOL=$(package)_asan ..
  endef
else ifeq ($(HOST), x86_64-apple-darwin16)
  define $(package)_config_cmds
    cmake -DCMAKE_C_COMPILER=$(darwin_CC) -DCMAKE_CXX_COMPILER=$(darwin_CXX) \
    -DCMAKE_INSTALL_PREFIX=$(host_prefix) -DCMAKE_BUILD_TYPE=$(package)_build_type \
    -DCMAKE_OSX_ARCHITECTURES=x86_64 -DCMAKE_SYSTEM_NAME=Darwin -DCMAKE_SYSTEM_PROCESSOR=x86_64 \
    -DCMAKE_C_COMPILER_TARGET=$(HOST) -DCMAKE_CXX_COMPILER_TARGET=$(HOST) \
    -DCMAKE_OSX_SYSROOT=$(OSX_SDK) -DTESTING=OFF \
    -DSHARED=OFF ..
  endef
else ifeq ($(HOST), x86_64-pc-linux-gnu)
  define $(package)_config_cmds
    cmake -DCMAKE_INSTALL_PREFIX=$(host_prefix) -DCMAKE_BUILD_TYPE=$(package)_build_type \
    -DTESTING=OFF -DSHARED=OFF ..
  endef
else
  define $(package)_config_cmds
    cmake -DCMAKE_C_COMPILER=$(HOST)-gcc -DCMAKE_CXX_COMPILER=$(HOST)-g++ \
    -DCMAKE_INSTALL_PREFIX=$(host_prefix) -DTESTING=OFF -DSHARED=OFF ..
  endef
endif

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef
