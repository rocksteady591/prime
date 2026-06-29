для сборки проекта должен быть установлен CMake, Conan
если конан установили в первый раз в терминале выполните команду conan profile detect --force
далее универсальная команда: conan install . --output-folder=build --build=missing --build=boost -s build_type=Debug -s arch=x86_64 -s compiler.cppstd=20 -c tools.cmake.cmaketoolchain:generator="Ninja"
cd build
cmake .. -G "Ninja" -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Debug
cmake --build .