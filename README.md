# AmiiBuddy

Initially started from [Wifiibo](https://github.com/Xerxes3rd/Wifiibo),
 but has since been radically changed. It gave me an "arduino ready" copy
 of [amiitool](https://github.com/socram8888/amiitool) to use.

Uses [M5ez](https://github.com/ropg/M5ez) for the user interface.

See it in action [from my preview post on reddit](https://www.reddit.com/r/Amiibomb/comments/hvc36t/it_was_suggested_i_post_this_here_several_weeks/).

## General info

Hardware Pieces:
- [M5Stack Faces Pocket Computer](https://m5stack.com/collections/m5-core/products/face)
- [M5Stack Proto Module](https://m5stack.com/collections/m5-module/products/proto-module)
- [Magic21x RFID Tag](https://www.google.com/url?q=https://lab401.com/collections/rfid-badges/products/ntag-compatible-21x&sa=D&source=hangouts&ust=1595436891659000&usg=AFQjCNEYJOh9Y4aKpK-neO_60FaWAG1ujA)
- [PN532 RFID Module](https://www.amazon.com/Module-Communication-Arduino-Raspberry-Android/dp/B082498VYD)
- 16 GB SD card (probably need much less, didn't realize how small all of this data really was... heh)

## How to make your own

1. 3D Print 1x `enclosure/M5Stack-PN532-Adapter.stl` and 2x `enclosure/M5Stack-PN532-top.stl`.
2. Buy the things.
3. Put the things together.
4. Put the amiibo files, powersaves, and key in the right places on an SD card.
   * amiibo files in `/library`, sub directories are encouraged.
   * powersaves in `/powersaves`, sub directories are not needed.
   * key in `/keys/retail.bin`
5. Put the latest release bin on the SD at `/firmware.bin`.
6. Put SD card into the M5Stack Core, and power it up.
7. Use the stock startup menu to select `/firmware.bin` to load.
8. Go to `Settings > Re-initialize database` to seed your database.
9. Enjoy!

## Compiling AmiiBuddy

1. Download the repo
2. Install [PlatformIO](https://platformio.org/)
3. Run a PlatformIO upload to compile and set up the environment
4. Edit `ESP32/main.cpp` and increase `usStackDepth` from `8192` to `16384`.
5. Run one more upload.

### Must use low SPIFF partitions

```
Advanced Memory Usage is available via "PlatformIO Home > Project Inspect"
RAM:   [=         ]  10.7% (used 56832 bytes from 532480 bytes)
Flash: [=======   ]  72.9% (used 1432974 bytes from 1966080 bytes)
```

### Edit ESP32/main.cpp

Increase usStackDepth to 16384 from 8192

### Memory management seems to be working

During a full rebuild...

```
src/m5stack/classes/AmiiboDatabaseManager.cpp:162 operator()(): Item counter:  368
src/m5stack/classes/AmiiboDatabaseManager.cpp:163 operator()(): File count:  751
src/m5stack/utils.cpp:159 printHeapUsage(): Heap usage:  140440 of 318924 (44.02% free)
src/m5stack/classes/AmiiboDatabaseManager.cpp:174 operator()(): Processing file:  /library/...
src/m5stack/utils.cpp:159 printHeapUsage(): Heap usage:  140440 of 318924 (44.02% free)
...
src/m5stack/classes/AmiiboDatabaseManager.cpp:162 operator()(): Item counter:  750
src/m5stack/classes/AmiiboDatabaseManager.cpp:163 operator()(): File count:  751
src/m5stack/utils.cpp:159 printHeapUsage(): Heap usage:  141096 of 319132 (44.20% free)
src/m5stack/classes/AmiiboDatabaseManager.cpp:174 operator()(): Processing file:  /library/...
src/m5stack/utils.cpp:159 printHeapUsage(): Heap usage:  141096 of 319132 (44.20% free)

```
