package=veriblock-pop-cpp
$(package)_version=bc2e4e09e15005b1e9da36a2cf3f5f17ed3722c9
$(package)_download_path=https://github.com/VeriBlock/alt-integration-cpp/archive/
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=4e5368468d3c86782c3f79f94a82a1f5c2c6d7fd69818192c3ac10ead2df58d9
$(package)_build_subdir=build
$(package)_build_type=$(BUILD_TYPE)
$(package)_asan=$(ASAN)

define $(package)_preprocess_cmds
  mkdir -p build
endef

define $(package)_config_cmds
  $($(package)_cmake) -DCMAKE_INSTALL_PREFIX=$(host_prefix) -DCMAKE_BUILD_TYPE=$(package)_build_type -DTESTING=OFF -DSHARED=OFF ..
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef
