# **building instructions for Nuked-SC55**

## CMake

**generic instructions for CMake**

### Ninja

#### **install prerequisites:**

`git cmake pkg-config ninja SDL2` and a c-compiler capable of C++11

#### **build:**

```
git clone --recurse-submodules https://github.com/nukeykt/Nuked-SC55.git
cd .\Nuked-SC55
cmake . -DCMAKE_BUILD_TYPE=Release -GNinja
cmake --build .
```

## Linux

T.B.D.

## MacOS

### Xcode


#### **install prerequisites (brew/macports - untested):**
```
git
cmake
SDL2
```
#### **build:**

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
in cmd:
```
winget install Git.Git
```

- ##### [vcpkg](https://github.com/microsoft/vcpkg):
in new cmd:
```
c:
cd %ProgramData%
git clone https://github.com/microsoft/vcpkg
cd vcpkg
setx VCPKG_PATH %cd%
setx PATH "%PATH%;%cd%"
.\bootstrap-vcpkg.bat -disableMetrics
```

in new admin-cmd:

`C:\ProgramData\vcpkg\vcpkg integrate install`

- ##### pkgconf:

in new cmd:
```
vcpkg install pkgconf
```

- ##### [SDL2](https://github.com/libsdl-org)

in new cmd:
```
vcpkg install SDL2 SDL2-image
```

#### **build:**

###### **example in new cmd:**

```
git clone --recurse-submodules https://github.com/nukeykt/Nuked-SC55.git
cd .\Nuked-SC55
cmake . -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -G"Visual Studio 17 2022"
cmake --build . --config Release
copy .\data\back.data .\Release
explorer .\Release
```
###### note: if SDL2 can't be found use

`setx SDL2_DIR %VCPKG_PATH%\installed\x64-windows\share\sdl2` (permanent, must reopen cmd to use) or

`set SDL2_DIR=%VCPKG_PATH%\installed\x64-windows\share\sdl2` (temporary, only for current cmd)



### MSYS2

[MSYS2](https://www.msys2.org/wiki/MSYS2-installation/)-**MSYS2 MinGW32** shell

#### **install prerequisites:**
```
pacman -S base-devel libtool pkg-config make gettext gcc git cmake mingw-w64-i686-gcc mingw-w64-x86_64-gcc mingw-w64-i686-cmake mingw-w64-x86_64-cmake mingw-w64-i686-pkg-config mingw-w64-x86_64-pkg-config mingw-w64-i686-toolchain mingw-w64-x86_64-toolchain mingw-w64-i686-SDL2 mingw-w64-x86_64-SDL2
```
note: you are asked twice to make a selection - just press "Return"/"Enter" to select all

#### **build:**

#### **use `msys2mingw32-build-release.sh`**

```
git clone --recurse-submodules https://github.com/nukeykt/Nuked-SC55.git
cd ./Nuked-SC55
sh ./msys2mingw32-build-release.sh
```
