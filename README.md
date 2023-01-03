# thesystem-cpp (temporary public release)

## Prerequisites

- Windows, MSVC Toolchain with clang-cl

- 7-zip (https://www.7-zip.org/) and Ninja (https://ninja-build.org/) must be installed and included in $PATH

  - (recommend installing it from Scoop (https://scoop.sh/))

## Build & Run

For the curious...

```
python configure.py --mode release
ninja
./build/release/bin/thesystem.exe
```