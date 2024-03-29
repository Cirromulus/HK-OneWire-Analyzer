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
|  1 | AC 1                    \
 ----                          |
|  2 | AC 2                    |- "DECK"
 ----                          |
|  3 | GND                     /
 ----
|  4 | GND                     \
 ----                          |
|  5 | POWER (logic level?)    |- "AMP
 ----                          |
|  6 | 12v                     /
 ----
|  7 | BUSY                    \
 ----                          |- HK OneWire
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

|  ID  | Name  |
| ---- | ----- |
| `0x0`| Probably the Tuner (or display?) |
| `0x3`| Probably the Tape Deck |
| `0x4`| ?? Only acted as destination, perhaps CD ?|
| `0x6`| ?? |

| Command | Type | Data |
| ------- | ---- | ---- |
| `0x0C`  | Set Time display | BCD-Like MM:SS, e.g. `0x0159` for 01:59 |

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
