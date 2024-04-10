# Building

## Linux

## MacOS

## Windows

### MSYS2

[MSYS2](https://www.msys2.org/wiki/MSYS2-installation/)-**MSYS2 MinGW32** shell

install prerequesites:
```
pacman -S base-devel libtool pkg-config make gettext gcc git cmake mingw-w64-i686-gcc mingw-w64-x86_64-gcc mingw-w64-i686-cmake mingw-w64-x86_64-cmake mingw-w64-i686-pkg-config mingw-w64-x86_64-pkg-config mingw-w64-i686-toolchain mingw-w64-x86_64-toolchain mingw-w64-i686-SDL2 mingw-w64-i686-SDL2_mixer mingw-w64-i686-SDL2_image mingw-w64-i686-SDL2_ttf mingw-w64-i686-SDL2_net mingw-w64-x86_64-SDL2 mingw-w64-x86_64-SDL2_mixer mingw-w64-x86_64-SDL2_image mingw-w64-x86_64-SDL2_ttf mingw-w64-x86_64-SDL2_net
```

use `msys2mingw32-build-release.sh`

```
git clone --recursive https://github.com/nukeykt/Nuked-SC55.git
cd ./Nuked-SC55
sh ./msys2mingw32-build-release.sh
```
