#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
BIN_DIR="${SCRIPT_DIR}/bin"
KDIR="${KDIR:-/lib/modules/$(uname -r)/build}"

echo "Preparing build directories..."
mkdir -p "${BUILD_DIR}/kernel" "${BUILD_DIR}/tools" "${BIN_DIR}"

echo "Staging kernel sources in ${BUILD_DIR}/kernel..."
cp -a "${SCRIPT_DIR}/src" "${BUILD_DIR}/kernel/"
cp -a "${SCRIPT_DIR}/include" "${BUILD_DIR}/kernel/"

cat << 'EOF' > "${BUILD_DIR}/kernel/Kbuild"
obj-m := int3drv.o
int3drv-y := src/init/device.o \
             src/init/module.o \
             src/core/context.o \
             src/core/resolve.o \
             src/core/validate.o \
             src/core/dispatch.o \
             src/mem/copy_read.o \
             src/mem/copy_write.o \
             src/mem/rw.o \
             src/mem/batch.o \
             src/mem/write_verify.o \
             src/ops/scalar.o \
             src/ops/probe.o \
             src/ops/scan.o \
             src/ops/scan_all.o \
             src/ops/vma.o \
             src/ops/vma_next.o \
             src/ops/maps_next.o \
             src/ops/proc_next.o \
             src/ops/modbase.o \
             src/ops/chain.o \
             src/ops/vscan.o \
             src/ops/cksum.o \
             src/ops/copy.o \
             src/ops/alloc.o \
             src/ops/thread.o \
             src/ops/stub.o \
             src/ops/deref.o

ccflags-y := -I$(src)/include -I$(src)/src -Wall -Wextra
EOF

echo "Compiling kernel module (int3drv.ko)..."
make -C "${KDIR}" M="${BUILD_DIR}/kernel" modules

cp -f "${BUILD_DIR}/kernel/int3drv.ko" "${BIN_DIR}/int3drv.ko"
cp -f "${BUILD_DIR}/kernel/int3drv.ko" "${SCRIPT_DIR}/int3drv.ko"

echo "Compiling userspace CLI (int3ctl, C23)..."
make -C "${SCRIPT_DIR}/tools"

echo "Build done."
echo "    Kernel Module : ${BIN_DIR}/int3drv.ko"
echo "    Userspace CLI : ${BIN_DIR}/int3ctl"
