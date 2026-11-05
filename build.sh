#!/bin/sh
# Cross-build AuthMee for Nokia N9 (Harmattan PR1.3, ARMv7 hard-float).
# TC: hdr/ (Ubuntu armhf cross headers + crt files), rt/ (Harmattan SDK libs and headers,
#     incl. Qt 4.7, QtMultimediaKit 1.2, OpenSSL 0.9.8), ld-arm (GNU ld wrapper).
# moc = Harmattan host-moc run under qemu-arm-static.
set -e
TC=${TC:-$HOME/tc}
D=$(cd "$(dirname "$0")" && pwd)
VER=${VER:-0.1.0}
OUT=${OUT:-$D/build}
mkdir -p $OUT
H=$TC/hdr/usr/arm-linux-gnueabihf
Q=$TC/rt/usr/include/qt4
L=$TC/rt/usr/lib
T="--target=arm-linux-gnueabihf -march=armv7-a -mtune=cortex-a8 -mfpu=neon -mfloat-abi=hard -mthumb"
BASE="$T -O2 -fno-pic -nostdinc -isystem $(clang -print-resource-dir)/include -U_FORTIFY_SOURCE -D_FILE_OFFSET_BITS=32"

# ZBar, QR decoder only (C, no _GNU_SOURCE: keeps glibc 2.39 headers from asking for C23 symbols)
for f in config error symbol image convert refcnt img_scanner scanner decoder misc stubs \
         decoder/qr_finder qrcode/bch15_5 qrcode/binarize qrcode/isaac qrcode/qrdec qrcode/qrdectxt qrcode/rs qrcode/util; do
  clang $BASE -isystem $H/include -std=gnu99 -w -include $D/zbar/config.h -I$D/zbar -I$D/zbar/include \
    -c $D/zbar/$f.c -o $OUT/zbar_$(basename $f).o
done

for h in store clock scanner ring; do
  QEMU_LD_PREFIX=$TC/rt qemu-arm-static -E LD_LIBRARY_PATH=/usr/lib:/lib $TC/rt/usr/bin/host-moc -I$Q $D/src/$h.h -o $OUT/moc_$h.cpp
done
CXX="$BASE -isystem $H/include/c++/13 -isystem $H/include/c++/13/arm-linux-gnueabihf -isystem $H/include"
for f in $D/src/main.cpp $D/src/otp.cpp $D/src/store.cpp $D/src/clock.cpp $D/src/scanner.cpp \
         $OUT/moc_store.cpp $OUT/moc_clock.cpp $OUT/moc_scanner.cpp $OUT/moc_ring.cpp; do
  clang++ $CXX -std=gnu++98 -fno-rtti -fno-exceptions -fno-threadsafe-statics -DQT_NO_DEBUG -DQT_SHARED -DAUTHMEE_VERSION=\"$VER\" \
    -I $Q -I $Q/QtCore -I $Q/QtGui -I $Q/QtNetwork -I $Q/QtDeclarative -I $Q/QtMultimediaKit -I $Q/QtMobility \
    -I $TC/inc -I $D/src -I $D/zbar/include -c $f -o $OUT/$(basename $f .cpp).o
done
G=$TC/hdr/usr/lib/gcc-cross/arm-linux-gnueabihf/13
clang++ $T --ld-path=$TC/ld-arm -no-pie -nostdlib -Wl,--allow-shlib-undefined -Wl,--as-needed -Wl,--hash-style=both -Wl,-z,norelro -Wl,-dynamic-linker,/lib/ld-linux.so.3 \
  $H/lib/crt1.o $H/lib/crti.o $G/crtbegin.o $OUT/*.o \
  $L/libQtMultimediaKit.so.1 $L/libQtDeclarative.so.4 $L/libQtNetwork.so.4 $L/libQtGui.so.4 $L/libQtCore.so.4 \
  $L/libcrypto.so.0.9.8 $L/libstdc++.so.6 $TC/rt/lib/libm.so.6 $TC/rt/lib/libpthread.so.0 $TC/rt/lib/librt.so.1 $TC/rt/lib/libc.so.6 $TC/rt/lib/libgcc_s.so.1 \
  $G/crtend.o $H/lib/crtn.o -o $OUT/authmee
llvm-strip $OUT/authmee
file $OUT/authmee
