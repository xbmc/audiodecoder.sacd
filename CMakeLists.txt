cmake_minimum_required(VERSION 3.5)
project(audiodecoder.sacd)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

find_package(Kodi REQUIRED)
find_package(Iconv REQUIRED)
find_package(TagLib REQUIRED)
find_package(WavPack REQUIRED)

include_directories(${PROJECT_SOURCE_DIR}/lib/libdsdpcm
                    ${KODI_INCLUDE_DIR}/..
                    ${iconv_INCLUDE_DIRS}
                    ${TAGLIB_INCLUDE_DIRS}
                    ${WAVPACK_INCLUDE_DIRS})

add_subdirectory(lib/libdsdpcm)

set(SACD_SOURCES src/Addon.cpp
                 src/SACDAudio.cpp
                 src/Settings.cpp)

set(SACD_HEADERS src/Addon.h
                 src/SACDAudio.h
                 src/Settings.h)

set(DEPLIBS dsdpcm ${WAVPACK_LIBRARIES} ${TAGLIB_LIBRARIES} ${ICONV_LIBRARY})

build_addon(audiodecoder.sacd SACD DEPLIBS)

include(CPack)
