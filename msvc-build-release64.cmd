@ECHO OFF
wget https://github.com/libsdl-org/SDL/releases/download/release-2.30.2/SDL2-devel-2.30.2-VC.zip
tar -xf SDL2-devel-2.30.2-VC.zip
del SDL2-devel-2.30.2-VC.zip
set SDL2_DIR %cd%\SDL2-2.30.2\cmake
cmake . -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -G"Visual Studio 17 2022"
cmake --build . --config Release
copy .\data\back.data .\Release
copy %SDL2_DIR%\..\lib\x64\SDL2.dll .\Release
move .\Release\nuked-sc55.exe .\Release\sc55emu.exe
start .\Release
