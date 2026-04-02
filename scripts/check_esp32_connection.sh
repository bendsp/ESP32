#!/bin/sh
set -eu

echo 'Serial devices:'
ls /dev/cu.* /dev/tty.* 2>/dev/null | sort || true

echo
echo 'USB devices (filtered):'
system_profiler SPUSBDataType | rg -i 'cp210|ch34|wch|usb serial|esp32|silicon labs|uart|devkit|espressif' -n -C 2 || true
