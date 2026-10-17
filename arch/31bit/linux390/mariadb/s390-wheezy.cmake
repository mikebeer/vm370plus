# CMake toolchain: 31-bit Debian 7 (wheezy) s390 guest on VM/370plus.
# wcc / wc++ are GCC 13 -m31 wrappers that compile and link against
# wheezy's glibc 2.13 (languages/common).  TGT holds the libraries we
# cross-build ourselves (ncurses).
get_filename_component(_H "${CMAKE_CURRENT_LIST_DIR}/../../languages/common" ABSOLUTE)
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR s390)
set(CMAKE_C_COMPILER   ${_H}/wcc)
set(CMAKE_CXX_COMPILER ${_H}/wc++)
set(CMAKE_AR      /usr/bin/s390x-linux-gnu-ar      CACHE FILEPATH "")
set(CMAKE_RANLIB  /usr/bin/s390x-linux-gnu-ranlib  CACHE FILEPATH "")
set(CMAKE_STRIP   /usr/bin/s390x-linux-gnu-strip   CACHE FILEPATH "")
if(NOT DEFINED ENV{WHEEZY})
  set(ENV{WHEEZY} /home/claude/src/cobol/wsys)
endif()
set(TGT $ENV{MDB_TGT})
set(CMAKE_FIND_ROOT_PATH $ENV{WHEEZY} ${TGT})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
set(CMAKE_LIBRARY_ARCHITECTURE s390-linux-gnu)
