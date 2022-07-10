cmake_minimum_required(VERSION 3.15.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
cmake_policy(SET CMP0057 NEW)

# option(BUILD_SHARED_LIBS "" ON)
add_definitions(-DCURL_STATICLIB)

#betterinfo-wrapper setup
file(
  GLOB_RECURSE SOURCE_FILES
  src/*.cpp
)

add_library(betterinfo-wrapper SHARED ${SOURCE_FILES})

target_include_directories(betterinfo-wrapper PRIVATE ${CMAKE_SOURCE_DIR}/libraries/curl/include)
target_link_libraries(betterinfo-wrapper ${CMAKE_SOURCE_DIR}/libraries/curl/libcurl_a.lib ws2_32 Crypt32 Wldap32 Normaliz)
target_link_options(betterinfo-wrapper PRIVATE "/OPT:REF,NOICF" "/NODEFAULTLIB:library")
#end betterinfo-wrapper

if (${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)
  # ensure 32 bit on clang
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -target i386-pc-windows-msvc")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -target i386-pc-windows-msvc")
  add_definitions("--target=i386-pc-windows-msvc")
endif()

add_subdirectory(libraries/cocos-headers)


# special thanks to this github issue: https://github.com/curl/curl/issues/5308 for helping me figure out how to link curl statically