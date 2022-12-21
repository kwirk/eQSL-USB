#!/bin/pwsh

if (!$ESPTOOL) {
  if (Get-Command esptool.py -errorAction SilentlyContinue) {
    $ESPTOOL="esptool.py"
  } elseif (Get-Command esptool -errorAction SilentlyContinue) {
    $ESPTOOL="esptool"
  } else {
    $ESPTOOL="./esptool"
  }
}

if ($args.Count -ge 1) {
  $PORT="--port", $args[0]
}

& $ESPTOOL --chip esp32s2 $PORT --baud 921600 --before default_reset --after no_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size  16MB 0x0 eQSL.ino.bootloader.bin 0x8000 eQSL.ino.partitions.bin 0xe000 boot_app0.bin 0x10000 eQSL.ino.bin
