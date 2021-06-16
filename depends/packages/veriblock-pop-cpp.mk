package=veriblock-pop-cpp
$(package)_version=2ef731621035cd1553aca3e8138758a616616ee1
$(package)_download_path=https://github.com/VeriBlock/alt-integration-cpp/archive/
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=03922bfa19736b6e543e6b37e9f6fd7cca739cd9ec17cbe9d84408606c5e279c
$(package)_build_subdir=build
$(package)_build_type=$(BUILD_TYPE)
$(package)_asan=$(ASAN)

define $(package)_preprocess_cmds
  mkdir -p build
endef

define $(package)_config_cmds
  $($(package)_cmake) -DTESTING=OFF -DSHARED=OFF ..
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef
