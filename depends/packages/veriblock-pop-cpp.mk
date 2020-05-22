package=veriblock-pop-cpp87f5afe62a40602e7d46cb4b17ce66328822d150ab3bd24f210515d7fe75e61f
$(package)_version=77abc5eed8d326d5e3c0f156160b72b0770136ee87f5afe62a40602e7d46cb4b17ce66328822d150ab3bd24f210515d7fe75e61f
$(package)_download_path=https://github.com/VeriBlock/alt-integration-cpp/archive/87f5afe62a40602e7d46cb4b17ce66328822d150ab3bd24f210515d7fe75e61f
$(package)_file_name=$($(package)_version).tar.gz87f5afe62a40602e7d46cb4b17ce66328822d150ab3bd24f210515d7fe75e61f
$(package)_sha256_hash=d35113b6ca55a2c9be5bc8c621135b0eff0427cabe03ee5fcd8c1d3b9440969487f5afe62a40602e7d46cb4b17ce66328822d150ab3bd24f210515d7fe75e61f
$(package)_build_subdir=build87f5afe62a40602e7d46cb4b17ce66328822d150ab3bd24f210515d7fe75e61f
$(package)_build_type=$(BUILD_TYPE)87f5afe62a40602e7d46cb4b17ce66328822d150ab3bd24f210515d7fe75e61f
$(package)_asan=$(ASAN)87f5afe62a40602e7d46cb4b17ce66328822d150ab3bd24f210515d7fe75e61f
87f5afe62a40602e7d46cb4b17ce66328822d150ab3bd24f210515d7fe75e61f
define $(package)_preprocess_cmds87f5afe62a40602e7d46cb4b17ce66328822d150ab3bd24f210515d7fe75e61f
  mkdir -p build87f5afe62a40602e7d46cb4b17ce66328822d150ab3bd24f210515d7fe75e61f
endef87f5afe62a40602e7d46cb4b17ce66328822d150ab3bd24f210515d7fe75e61f
87f5afe62a40602e7d46cb4b17ce66328822d150ab3bd24f210515d7fe75e61f
ifeq ($(strip $(HOST)),)87f5afe62a40602e7d46cb4b17ce66328822d150ab3bd24f210515d7fe75e61f
  define $(package)_config_cmds87f5afe62a40602e7d46cb4b17ce66328822d150ab3bd24f210515d7fe75e61f
    cmake -DCMAKE_INSTALL_PREFIX=$(host_prefix) -DCMAKE_BUILD_TYPE=$(package)_build_type \87f5afe62a40602e7d46cb4b17ce66328822d150ab3bd24f210515d7fe75e61f
    -DTESTING=OFF -DSHARED=OFF -DASAN:BOOL=$(package)_asan ..87f5afe62a40602e7d46cb4b17ce66328822d150ab3bd24f210515d7fe75e61f
  endef87f5afe62a40602e7d46cb4b17ce66328822d150ab3bd24f210515d7fe75e61f
else ifeq ($(HOST), x86_64-apple-darwin16)87f5afe62a40602e7d46cb4b17ce66328822d150ab3bd24f210515d7fe75e61f
  define $(package)_config_cmds87f5afe62a40602e7d46cb4b17ce66328822d150ab3bd24f210515d7fe75e61f
    cmake -DCMAKE_C_COMPILER=$(darwin_CC) -DCMAKE_CXX_COMPILER=$(darwin_CXX) \87f5afe62a40602e7d46cb4b17ce66328822d150ab3bd24f210515d7fe75e61f
    -DCMAKE_INSTALL_PREFIX=$(host_prefix) -DCMAKE_BUILD_TYPE=$(package)_build_type \87f5afe62a40602e7d46cb4b17ce66328822d150ab3bd24f210515d7fe75e61f
    -DCMAKE_OSX_ARCHITECTURES=x86_64 -DCMAKE_SYSTEM_NAME=Darwin -DCMAKE_SYSTEM_PROCESSOR=x86_64 \87f5afe62a40602e7d46cb4b17ce66328822d150ab3bd24f210515d7fe75e61f
    -DCMAKE_C_COMPILER_TARGET=$(HOST) -DCMAKE_CXX_COMPILER_TARGET=$(HOST) \87f5afe62a40602e7d46cb4b17ce66328822d150ab3bd24f210515d7fe75e61f
    -DCMAKE_OSX_SYSROOT=$(OSX_SDK) -DTESTING=OFF \87f5afe62a40602e7d46cb4b17ce66328822d150ab3bd24f210515d7fe75e61f
    -DSHARED=OFF ..87f5afe62a40602e7d46cb4b17ce66328822d150ab3bd24f210515d7fe75e61f
  endef87f5afe62a40602e7d46cb4b17ce66328822d150ab3bd24f210515d7fe75e61f
else ifeq ($(HOST), x86_64-pc-linux-gnu)87f5afe62a40602e7d46cb4b17ce66328822d150ab3bd24f210515d7fe75e61f
  define $(package)_config_cmds87f5afe62a40602e7d46cb4b17ce66328822d150ab3bd24f210515d7fe75e61f
    cmake -DCMAKE_INSTALL_PREFIX=$(host_prefix) -DCMAKE_BUILD_TYPE=$(package)_build_type \87f5afe62a40602e7d46cb4b17ce66328822d150ab3bd24f210515d7fe75e61f
    -DTESTING=OFF -DSHARED=OFF ..87f5afe62a40602e7d46cb4b17ce66328822d150ab3bd24f210515d7fe75e61f
  endef87f5afe62a40602e7d46cb4b17ce66328822d150ab3bd24f210515d7fe75e61f
else87f5afe62a40602e7d46cb4b17ce66328822d150ab3bd24f210515d7fe75e61f
  define $(package)_config_cmds87f5afe62a40602e7d46cb4b17ce66328822d150ab3bd24f210515d7fe75e61f
    cmake -DCMAKE_C_COMPILER=$(HOST)-gcc -DCMAKE_CXX_COMPILER=$(HOST)-g++ \87f5afe62a40602e7d46cb4b17ce66328822d150ab3bd24f210515d7fe75e61f
    -DCMAKE_INSTALL_PREFIX=$(host_prefix) -DTESTING=OFF -DSHARED=OFF ..87f5afe62a40602e7d46cb4b17ce66328822d150ab3bd24f210515d7fe75e61f
  endef87f5afe62a40602e7d46cb4b17ce66328822d150ab3bd24f210515d7fe75e61f
endif87f5afe62a40602e7d46cb4b17ce66328822d150ab3bd24f210515d7fe75e61f
87f5afe62a40602e7d46cb4b17ce66328822d150ab3bd24f210515d7fe75e61f
define $(package)_build_cmds87f5afe62a40602e7d46cb4b17ce66328822d150ab3bd24f210515d7fe75e61f
  $(MAKE)87f5afe62a40602e7d46cb4b17ce66328822d150ab3bd24f210515d7fe75e61f
endef87f5afe62a40602e7d46cb4b17ce66328822d150ab3bd24f210515d7fe75e61f
87f5afe62a40602e7d46cb4b17ce66328822d150ab3bd24f210515d7fe75e61f
define $(package)_stage_cmds87f5afe62a40602e7d46cb4b17ce66328822d150ab3bd24f210515d7fe75e61f
  $(MAKE) DESTDIR=$($(package)_staging_dir) install87f5afe62a40602e7d46cb4b17ce66328822d150ab3bd24f210515d7fe75e61f
endef87f5afe62a40602e7d46cb4b17ce66328822d150ab3bd24f210515d7fe75e61f
