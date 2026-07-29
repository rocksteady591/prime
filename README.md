для сборки проекта должен быть установлен CMake, Conan
если конан установили в первый раз в терминале выполните команду conan profile detect --force

далее универсальная команда: conan install . --output-folder=build --build=missing --build=boost -s build_type=Debug -s arch=x86_64 -s compiler.cppstd=23 -c tools.cmake.cmaketoolchain:generator="Ninja"

cd build

под все системы: cmake .. -G "Ninja" -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Debug или cmake --preset conan-debug

под xcode: cmake .. -G Xcode \
  -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++

cmake --build .
