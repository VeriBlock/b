package=veriblock-pop-cpp
$(package)_version=2ef731621035cd1553aca3e8138758a616616ee1
$(package)_download_path=https://github.com/VeriBlock/alt-integration-cpp/archive/
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=03922bfa19736b6e543e6b37e9f6fd7cca739cd9ec17cbe9d84408606c5e279c
$(package)_build_subdir=build
$(package)_asan=$(ASAN)

ifeq (debug,$(release_type))
$(package)_build_type=Debug
else
$(package)_build_type=Release
endif

define $(package)_preprocess_cmds
  mkdir -p build
endef

define $(package)_config_cmds
  $($(package)_cmake) -DCMAKE_INSTALL_PREFIX=$(host_prefix) -DTESTING=OFF -DSHARED=OFF -DCMAKE_BUILD_TYPE=$($(package)_build_type) ..
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef
