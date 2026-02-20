# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "")
  file(REMOVE_RECURSE
  "CMakeFiles\\PyRoomStudio_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\PyRoomStudio_autogen.dir\\ParseCache.txt"
  "PyRoomStudio_autogen"
  )
endif()
