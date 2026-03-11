cmake -S . -B build \
    -DCMAKE_PREFIX_PATH="<path/to/Qt/6.x/macos>" \
    -DCMAKE_BUILD_TYPE=Release \
    -G Ninja

cmake --build build
