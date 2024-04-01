# Analyzer for the Harman / Kardon OneWire-like communication protocol in the Festival 500 sound system.

TODO: More description. WIP.

## Summary

The Festival 300 / 500 sound system consists of the Tuner (master), the Amplifyer, a CD deck and a tape deck.
The goal of this repository is to decode the serial communication bus used for controlling the units.
Currently, it can decode the commands, but not yet associate the transactions (TODO).

## Hardware layout

The Pin header is structured as follows:
```
 ---
|  1 | AC 1   -                \
 ----          |- 18v AC       |
|  2 | AC 2   -                |- "DECK"
 ----                          |
|  3 | GND (Power)             /
 ----
|  4 | GND (COM)               \
 ----                          |
|  5 | POWER (logic level?)    |- "AMP
 ----                          |
|  6 | 12v                     /
 ----
|  7 | BUSY                    \
 ----                          |- HK OneWire (5v)
|  8 | DATA                    /
 ----
|  9 | AC 1                    \
 ----                          |
| 10 | GND                     |
 ----                          |
| 11 | AC 2                    |- "CD"
 ----                          |
| 12 | AC 1 (yes, 1 again)     |
 ----                          |
| 13 | 5.4v                    /
 ----
```
Source: from the marvellous repair guide that I probably can't share publicly.

## Low-Level command structure

TODO. See code.

## High-Level transactions

TODO: More. Also perhaps build an [extension](https://support.saleae.com/extensions/high-level-analyzer-extensions) for it.

Note: All numbers in Hex

Recorded IDs
----

|  ID  | Name  |
| ---- | ----- |
| `0`| Probably the Tuner (or display?) |
| `3`| Probably the Tape Deck |
| `4`| ?? Only acted as destination, perhaps CD, or AMP ?|
| `6`| only startup and off: `6->6 0x20` |

Commands `0 -> 3`
----

Missing to identify: Mute / unmute output

|  ID  | Description | (Data)
| ---- | ----------- | ------
| `05` | Stop ?? Comes directly after `0->4: 07` and after startup| 
| `07` | Eject ? |
| `08` | Increase FF speed |
| `09` | Increase FR speed |
| `0A` | Fast Forward ? |
| `0B` | Fast Reverse ? |
| `0E` | Dolby: B |
| `0F` | Dolby: C |
| `10` | Dolby: None |
| `11` | Reverse Playing direction
| `12` | ?? Comes directly after `08` or `09`|
| `13` | ?? Comes directly after startup
| `15` | Request to record
| `17` | Play |
| `16` | Pause (Mute?)|
| `1C` | ? Answer to `3->0 : 07`, so perhaps "Ok to output", or "request for playing direction"
| `1E` | RECORD "NORM"
| `1F` | Zero time counter

Commands `3 -> 0`
----

|  ID  | Description | (Data)
| ---- | ----------- | ------
| `05` | ? Comes sometimes after `13` (is it the broadcast stop?)|
| `06` | Status Un-Playable? Eject was pressed, tape coming out
| `07` | Status    playable? Eject was pressed, tape coming in and small wait time
| `0B` | Current FF/FR speed | _Only one byte!_ : lower nibble = speed (1-4), MSBit (`80`) = isReverse |
| `0C` | Set Time display | BCD-Like MM:SS, e.g. `0x0159` for 01:59 |
| `0D` | Set Time display Negative ? | " |
| `0F` | Tape deck is present (in Dolby: C mode?) |
| `10` | Is playing "to the right" |
| `11` | Is playing "to the left" |
| `12` | ?? comes sometimes as answer to `0 -> 3: 12`
| `13` | ? Came during FF, and after startup |
| `14` | Can not record (tape was not loaded) |
| `15` | Able to record (tape was just loaded) |

Broadcast commands `0 -> 0`
| Command | Direction | Type | Data |
| ------- | --------- | ---- | ---- |
|   `01`  |  `0->0`   | System On
|   `02`  |  `0->0`   | System Off
|   `05`  |  `0->0`   | Stop all?


Other commands
----

| Command | Direction | Type | Data |
| ------- | --------- | ---- | ---- |
|   `13`  |  `0->4`   | ? Amp set input to Tape in ?
|   `06`  |  `0->4`   | ? Amp set off ?
|   `07`  |  `0->4`   | ? Amp set mute ?l



## Updating an Existing Analyzer to use CMake & GitHub Actions

If you maintain an existing C++ analyzer, or wish to fork and update someone else's analyzer, please follow these steps.

1. Delete the contents of the existing repository, except for the source directory, and the readme.
2. Copy the contents of this sample repository into the existing analyzer, except for the src and docs directories, or the rename_analyzer.py script. The `.clang-format` file is optional, it would allow you to auto-format your code to our style using [clang-format](https://clang.llvm.org/docs/ClangFormat.html).
3. Rename the existing source directory to src. This is optional, but it might make future updates from this sample analyzer easier to roll out. Make sure the CMakeLists.txt file reflects your source path.
4. In the new CMakeLists.txt file, make the following changes:

- In the line `project(SimpleSerialAnalyzer)`, replace `SimpleSerialAnalyzer` with the name of the existing analyzer, for example `project(I2CAnalyzer)`
- In the section `set(SOURCES`, replace all of the existing source code file names with the file names of the existing source code.

5. Update the readme! Feel free to just reference the SampleAnalyzer repository, or copy over the build instructions.
6. Try the build instructions to make sure the analyzer still builds, or commit this to GitHub to have GitHub actions build it for you!
7. Once you're ready to create a release, add a tag to your last commit to trigger GitHub to publish a release.
