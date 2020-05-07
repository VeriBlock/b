package=veriblock-pop-cpp
<<<<<<< Updated upstream
$(package)_version=e8a52a49091d9c1de97bbf77de6a2f5e616ce3cc
$(package)_download_path=https://github.com/VeriBlock/alt-integration-cpp/archive/
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=2958615d4b1e7884f8ab429d3c74369b55120caa85838e3879b2f853942cfe82
=======
$(package)_version=56250fa96b5e8eba99933b7a27ac0babe09b4f91
$(package)_download_path=https://github.com/VeriBlock/alt-integration-cpp/archive/
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=c144b9eea8a7d9bdcee49aa7a1fe156043a1f2b011cdeba5be776939771ba594
$(package)_build_subdir=build

define $(package)_preprocess_cmds
  mkdir -p build
endef
>>>>>>> Stashed changes

ifeq ($(HOST), x86_64-w64-mingw32)
  define $(package)_config_cmds
    cmake -DCMAKE_C_COMPILER=$(HOST)-gcc -DCMAKE_CXX_COMPILER=$(HOST)-g++ -DCMAKE_INSTALL_PREFIX=$($(package)_staging_dir)$(host_prefix) -DCMAKE_BUILD_TYPE=Release -DTESTING=OFF -DWITH_ROCKSDB=OFF -DSHARED=OFF -B .
  endef
else
  define $(package)_config_cmds
    cmake -DCMAKE_INSTALL_PREFIX=$($(package)_staging_dir)$(host_prefix) -DCMAKE_BUILD_TYPE=Release -DTESTING=OFF -DWITH_ROCKSDB=OFF -DSHARED=OFF -B .
  endef
endif

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) install
endef