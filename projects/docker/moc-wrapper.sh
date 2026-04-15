#!/bin/sh
# Qt6 moc wrapper — 在 ARM64 主机上通过 QEMU 运行 x86_64 moc
exec /usr/bin/qemu-x86_64-static /usr/lib/qt6/libexec/moc "${@}"
