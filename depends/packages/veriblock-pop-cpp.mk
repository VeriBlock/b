package=veriblock-pop-cpp
$(package)_version=f6a54b04a18439826f72d8d5b70afe4109db37ee
$(package)_download_path=https://github.com/VeriBlock/alt-integration-cpp/archive/
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=78ec98ef8cf4b8549fd15dc62770cbf840306833b4cf9a8c6b93ca69f7771a7e
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
