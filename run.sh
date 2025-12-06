#!/bin/bash

# --- Build kernel ---
cd kernel
make clean
make -j$(nproc)
if [ $? -ne 0 ]; then
    echo "Kernel Build Failed! Terminating.."
    exit 1
fi
cd ..

# --- Ensure logs directory exists ---
mkdir -p logs

# --- Run QEMU headless (no graphics, no X11, no GDB) ---
qemu-system-x86_64 \
    -cdrom bin/koizos.iso \
    -serial stdio \
    -no-reboot \
    -no-shutdown
