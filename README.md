чтобы собрать
из корневой папки conan install . -s build_type=Debug --output-folder=build --build=missing
переходим в директорию cd build
запускаем cmake .. --DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake --DCMAKE_BUILD_TYPE=Debug
собираем cmake --build .
