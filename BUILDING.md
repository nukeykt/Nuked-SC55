# Building

## Linux

## MacOS

### Xcode


**install prerequisites (brew/macports - untested):**
```
git
cmake
SDL2
SDL2_image
```

open terminal.app

`git clone --recurse-submodules https://github.com/nukeykt/Nuked-SC55.git`

create an Xcode project:

```
$ cd Nuked-SC55
$ cmake -G Xcode .
```

- open created Xcode project
- Product -> Scheme -> Edit Scheme -> Build Configuration set to Release
- build
- copy data/back.data the same directory as built binary


## Windows

### VisualStudio 2022

#### **install prerequisites:**
- ##### Visual Studio with Windows SDK and [CMake](https://learn.microsoft.com/en-us/cpp/build/cmake-projects-in-visual-studio?view=msvc-170)
  
- ##### git:
```
winget install Git.Git
```

- ##### [vcpkg](https://github.com/microsoft/vcpkg):
  
in cmd:
```
git clone https://github.com/microsoft/vcpkg
.\vcpkg\bootstrap-vcpkg.bat -disableMetrics
```
in admin cmd:

`.\vcpkg\vcpkg integrate install`

add vcpkg-folder to %PATH%

- ##### [SDL2](https://github.com/libsdl-org)
###### Get SDL2

`vcpkg install SDL2 SDL2-image SDL2-mixer`

- ##### build
###### **Example in cmd**

```
cd..
git clone --recurse-submodules https://github.com/nukeykt/Nuked-SC55.git
cd .\Nuked-SC55
cmake . -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -G"Visual Studio 17 2022"
cmake --build . --config Release
echo copy SDL2 %SDL2_DIR%\lib\x64\SDL2.dll to %cd%\Release
copy .\data\back.data .\Release
explorer .\Release
```


### MSYS2

[MSYS2](https://www.msys2.org/wiki/MSYS2-installation/)-**MSYS2 MinGW32** shell

#### **install prerequisites:**
```
pacman -S base-devel libtool pkg-config make gettext gcc git cmake ninja mingw-w64-x86_64-ninja mingw-w64-i686-ninja mingw-w64-i686-gcc mingw-w64-x86_64-gcc mingw-w64-i686-cmake mingw-w64-x86_64-cmake mingw-w64-i686-pkg-config mingw-w64-x86_64-pkg-config mingw-w64-i686-toolchain mingw-w64-x86_64-toolchain mingw-w64-i686-SDL2 mingw-w64-i686-SDL2_mixer mingw-w64-i686-SDL2_image mingw-w64-i686-SDL2_ttf mingw-w64-i686-SDL2_net mingw-w64-x86_64-SDL2 mingw-w64-x86_64-SDL2_mixer mingw-w64-x86_64-SDL2_image mingw-w64-x86_64-SDL2_ttf mingw-w64-x86_64-SDL2_net
```
Note: you are asked twice to make a selection - just press "Return"/"Enter" to select all


#### **use `msys2mingw32-build-release.sh`**

```
git clone --recurse-submodules https://github.com/nukeykt/Nuked-SC55.git
cd ./Nuked-SC55
sh ./msys2mingw32-build-release.sh
```
