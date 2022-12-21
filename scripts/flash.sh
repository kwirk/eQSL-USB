#!/bin/bash

if [ -z ${ESPTOOL+x} ]; then
  if which esptool.py &>/dev/null; then
    ESPTOOL=esptool.py
  elif which esptool &>/dev/null; then
    ESPTOOL=esptool
  else
    ESPTOOL=./esptool
  fi
fi

if [ "$#" -ge 1 ]; then
  PORT="--port ${1}"
fi

$ESPTOOL --chip esp32s3 $PORT --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size 16MB 0x0 eQSL.ino.bootloader.bin 0x8000 eQSL.ino.partitions.bin 0xe000 boot_app0.bin 0x10000 eQSL.ino.bin
