FROM conanio/gcc11

RUN apt update && apt install -y

COPY conanfile.txt /app/

RUN conan install . --output-folder=build --build=missing -s build_type=Debug

COPY CMakeLists.txt /app/
COPY ./src /app/src

RUN cd /app/build && \
    cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Debug && \
    cmake --build .

CMD ./app/build/bin/primal_server