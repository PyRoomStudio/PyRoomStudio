cmake -S . -B build \
        -DCMAKE_PREFIX_PATH="C:/Qt/6.10.2/mingw_64" \
        -DCMAKE_CXX_COMPILER="C:/Qt/Tools/mingw1310_64/bin/g++.exe" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_DISABLE_FIND_PACKAGE_WrapVulkanHeaders=ON \
        -G Ninja
        
cmake --build build