# eQSL USB

This repo provides the software to create a USB memory device which automatically downloads new electronic QSL cards over WiFi from [eQSL.cc™](https://www.eqsl.cc), as well as removing cards older than a configurable age.
This enables use of most digital photo frames to display your most recent electronic QSL cards.

![T-Dongle showing eQSL card being downloaded](https://github.com/kwirk/eQSL-USB/raw/main/img/dongle.jpg) ![Digital photo frame showing eQSL card](https://github.com/kwirk/eQSL-USB/raw/main/img/frame.jpg)

This project is not associated with eQSL.cc™ in any way, but makes use of your account and the services they offer. If not already, please consider supporting eQSL.cc™ via their memberships.

## Installing

Below is the steps to create your own eQSL USB mass storage device.

### Hardware

This is designed to run on the LILYGO® T-Dongle ESP32-S2 Development Board. These are available on [AliExpress™](https://www.aliexpress.com/item/1005004241114226.html) and other online stores. This dongle has a USB type A connector which should work with many digital photo frames (note that on some frames, you may not be able to use the included case as it is a little bulky; but the device works fine without it).

A micro SD Card is required. The images are small, so a large card isn't required. A blank card is required as the software will delete any files which names do not match current QSL cards you have (however, it shouldn't modify content of folders on the SD card).

And finally a digital photo frame to display the images. Most that have a USB Type A port should work, and can be picked up very cheaply second hand / used.

### Software

You will need to install or download [*esptool*](https://github.com/espressif/esptool/releases/latest) for your operating system.

In the [Releases page](https://github.com/kwirk/eQSL-USB/releases/latest) you'll find a ZIP file containing necessary binary images and scripts which will flash the T-Dongle.

Before running the script, on the T-Dongle hold down the boot button (labeled **BOT**) as you insert it to a USB port on your computer.

If you have download *esptool* ZIP file per above, you can simply drop the *esptool* executable in the same folder as the binaries and scripts, and run the script for your operating system.

Once the script finishes flashing you can remove the T-Dongle from your computer, and begin configuration per below instructions.

### Configuration

Before using, you will need to go to your eQSL.cc™ *Profile* page and change *eQSL Display Format* to *JPG*.

Inserting your SD card directly into your computer, you should copy [config.ini](https://gitcdn.link/cdn/kwirk/eQSL-USB/main/config.ini) onto the root of the SD card. Edit this file, adding your WiFi and eQSL.cc™ credentials.

Once done insert this card into the T-Dongle SD card slot, and it is now ready to use. The screen on the T-Dongle will provide status messages showing progress, and any error messages should anything go wrong.

## Developing

To build the software for flashing to the T-Dongle, you need Arduino IDE and with ***ESP32 Arduino core v2.0.0***.

Included custom versions in `libraries` folder (version is base version that was modified)

| Name         | Version | Customisation |
| ------------ | ------- | ------------- |
| TFT\_eSPI    | 2.4.61  | `User_Setup.h` customised for TFT display |
| ESP32TinyUSB | 1.3.4   | Modified to make USB interface read-only and enable reboot |

And other libraries without modification (version numbers as tested, may work with other versions)

| Name      | Version  |
| --------- | -------- |
| IniFile   | ~1.3.0   |
| Regexp    | ~0.1.0   |
| SD        | ~1.2.4   |
| U8g2      | ~2.32.15 |
| UrlEncode | ~1.0.0   |

## License

Copyright (C) 2022 Steven Hiscocks

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
