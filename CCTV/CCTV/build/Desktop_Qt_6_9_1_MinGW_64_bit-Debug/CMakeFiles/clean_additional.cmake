# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CCTV_Client_autogen"
  "CMakeFiles\\CCTV_Client_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\CCTV_Client_autogen.dir\\ParseCache.txt"
  )
endif()
