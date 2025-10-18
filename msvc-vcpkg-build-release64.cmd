cmake . -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -G"Visual Studio 17 2022"
cmake --build . --config Release
copy .\data\back.data .\Release
move .\Release\nuked-sc55.exe .\Release\sc55emu.exe
explorer .\Release