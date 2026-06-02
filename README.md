для сборки проекта должен быть установлен CMake, Conan
если конан установили в первый раз в терминале выполните команду conan profile detect --force
далее в корневой директории проекта выполнить conan install . --output-folder=build --build=missing -s build_type=Debug или Release если надо релиз
и последнее cmake --preset conan-default
