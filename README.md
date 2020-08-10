# AmiiBuddy

Initially started from [Wifiibo](https://github.com/Xerxes3rd/Wifiibo),
 but has since been radically changed. It gave me an "arduino ready" copy
 of [amiitool](https://github.com/socram8888/amiitool) to use.

Uses [M5ez](https://github.com/ropg/M5ez) for the user interface.

See it in action [from my preview post on reddit](https://www.reddit.com/r/Amiibomb/comments/hvc36t/it_was_suggested_i_post_this_here_several_weeks/).

## General info

Hardware Pieces:
- [M5Stack Faces Pocket Computer](https://m5stack.com/collections/m5-core/products/face)
- [Magic21x RFID Tag](https://www.google.com/url?q=https://lab401.com/collections/rfid-badges/products/ntag-compatible-21x&sa=D&source=hangouts&ust=1595436891659000&usg=AFQjCNEYJOh9Y4aKpK-neO_60FaWAG1ujA)
- [PN532 RFID Module](https://www.amazon.com/Module-Communication-Arduino-Raspberry-Android/dp/B082498VYD)
- 16 GB SD card (probably need much less, didn't realize how small all of this data really was... heh)

## How to make your own

1. Buy the things.
2. Put the things together.
3. Put the amiibo files and key in the right places on an SD card.
4. Put SD card into the M5Stack.
5. Download the repo
6. Install [PlatformIO](https://platformio.org/)
7. Run PlatformIO to compile and load the software

More details coming later...

## Must use low SPIFF partitions

```
Advanced Memory Usage is available via "PlatformIO Home > Project Inspect"
RAM:   [=         ]  10.1% (used 54032 bytes from 532480 bytes)
Flash: [=======   ]  72.3% (used 1421010 bytes from 1966080 bytes)
```