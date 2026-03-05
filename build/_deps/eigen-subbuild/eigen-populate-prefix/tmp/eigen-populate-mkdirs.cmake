# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/Users/Ematt/Documents/PyRoomStudio/pyroomstudio-cpp/build/_deps/eigen-src"
  "C:/Users/Ematt/Documents/PyRoomStudio/pyroomstudio-cpp/build/_deps/eigen-build"
  "C:/Users/Ematt/Documents/PyRoomStudio/pyroomstudio-cpp/build/_deps/eigen-subbuild/eigen-populate-prefix"
  "C:/Users/Ematt/Documents/PyRoomStudio/pyroomstudio-cpp/build/_deps/eigen-subbuild/eigen-populate-prefix/tmp"
  "C:/Users/Ematt/Documents/PyRoomStudio/pyroomstudio-cpp/build/_deps/eigen-subbuild/eigen-populate-prefix/src/eigen-populate-stamp"
  "C:/Users/Ematt/Documents/PyRoomStudio/pyroomstudio-cpp/build/_deps/eigen-subbuild/eigen-populate-prefix/src"
  "C:/Users/Ematt/Documents/PyRoomStudio/pyroomstudio-cpp/build/_deps/eigen-subbuild/eigen-populate-prefix/src/eigen-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/Ematt/Documents/PyRoomStudio/pyroomstudio-cpp/build/_deps/eigen-subbuild/eigen-populate-prefix/src/eigen-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/Users/Ematt/Documents/PyRoomStudio/pyroomstudio-cpp/build/_deps/eigen-subbuild/eigen-populate-prefix/src/eigen-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
