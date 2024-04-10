#/bin/sh
cmake . -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -G "MinGW Makefiles"
mingw32-make
mkdir ./release
cp ./nuked-sc55.exe ./release/SC55mkII.exe
cp ./back.data ./release/back.data
cp ./README.md ./release/README.md
cp /mingw32/bin/libgcc_s_dw2-1.dll ./release/libgcc_s_dw2-1.dll
cp /mingw32/bin/libstdc++-6.dll ./release/libstdc++-6.dll
cp /mingw32/bin/SDL2.dll ./release/SDL2.dll
start release
cd ..
