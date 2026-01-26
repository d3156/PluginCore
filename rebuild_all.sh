
rm -rf ./build
rm -rf _CPack_Packages
rm -rf release
rm d3156*

mkdir release

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
cpack --config build/CPackConfig.cmake -G DEB

sudo dpkg -i d3156*
mv d3156* ./release/

cd PluginLoader

rm -rf ./build

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

mv ./build/PluginLoader* ./../release/
