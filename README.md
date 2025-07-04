# Let-s-code

This project is now organized as a small CMake based application. To build it run:

##  Linux Native Build

make sure you using fltk1.4.3

```bash
mkdir build && cd build
cmake ..
make
```

The resulting executable `lets_code` will be placed in the `build` directory.

## Cross-Compile for Windows (from Linux)

To build a Windows .exe version on Linux using MinGW:

```
Install Dependencies (Debian/Ubuntu)

sudo apt install mingw-w64 cmake g++-mingw-w64

mkdir build-win && cd build-win

cmake .. \
  -DCMAKE_TOOLCHAIN_FILE=../mingw-toolchain.cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_EXE_LINKER_FLAGS="-static-libgcc -static-libstdc++"

  make
  ```
  You will get lets_code.exe in build-win/.