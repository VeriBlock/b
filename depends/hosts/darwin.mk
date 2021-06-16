OSX_MIN_VERSION=10.12
OSX_SDK_VERSION=10.14
OSX_SDK=$(SDK_PATH)/MacOSX$(OSX_SDK_VERSION).sdk
LD64_VERSION=253.9

darwin_CC=env -u C_INCLUDE_PATH -u CPLUS_INCLUDE_PATH \
              -u OBJC_INCLUDE_PATH -u OBJCPLUS_INCLUDE_PATH -u CPATH \
              -u LIBRARY_PATH \
            clang --target=$(host) -mmacosx-version-min=$(OSX_MIN_VERSION) \
              -mlinker-version=$(LD64_VERSION) \
              --sysroot=$(OSX_SDK)
darwin_CXX=env -u C_INCLUDE_PATH -u CPLUS_INCLUDE_PATH \
               -u OBJC_INCLUDE_PATH -u OBJCPLUS_INCLUDE_PATH -u CPATH \
               -u LIBRARY_PATH \
             clang++ --target=$(host) -mmacosx-version-min=$(OSX_MIN_VERSION) \
               -mlinker-version=$(LD64_VERSION) \
               --sysroot=$(OSX_SDK) \
               -stdlib=libc++

darwin_CFLAGS=-pipe
darwin_CXXFLAGS=$(darwin_CFLAGS)

darwin_release_CFLAGS=-O2
darwin_release_CXXFLAGS=$(darwin_release_CFLAGS)

darwin_debug_CFLAGS=-O1
darwin_debug_CXXFLAGS=$(darwin_debug_CFLAGS)

darwin_native_toolchain=native_cctools
