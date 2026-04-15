#!/bin/sh
# QEMU x86_64 wrapper for running x86_64 binaries on ARM64 host
# 用法: qemu-wrapper.sh <x86_64二进制> [参数...]
QEMU_X86_64="/usr/bin/qemu-x86_64-static"
BINARY="$1"
shift
exec "$QEMU_X86_64" "$BINARY" "$@"
