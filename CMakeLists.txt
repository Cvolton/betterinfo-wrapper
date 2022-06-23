cmake_minimum_required(VERSION 3.10.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
cmake_policy(SET CMP0057 NEW)

# option(BUILD_SHARED_LIBS "" ON)

#betterinfo-wrapper setup
file(
  GLOB_RECURSE SOURCE_FILES
  src/*.cpp
)

add_library(betterinfo-wrapper SHARED ${SOURCE_FILES})

target_include_directories(betterinfo-wrapper PRIVATE libraries/minhook/include libraries/gd.h libraries/gd.h/include)
target_link_libraries(betterinfo-wrapper minhook cocos2d)
target_link_options(betterinfo-wrapper PRIVATE "/OPT:REF,NOICF")
#end betterinfo-wrapper

if (${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)
  # ensure 32 bit on clang
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -target i386-pc-windows-msvc")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -target i386-pc-windows-msvc")
  add_definitions("--target=i386-pc-windows-msvc")
endif()

add_subdirectory(libraries/minhook)
add_subdirectory(libraries/cocos-headers)