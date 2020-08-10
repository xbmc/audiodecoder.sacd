cmake_minimum_required(VERSION 3.5)
project(audiodecoder.sacd)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

find_package(Kodi REQUIRED)
find_package(Iconv REQUIRED)
find_package(WavPack REQUIRED)

include_directories(${PROJECT_SOURCE_DIR}/lib/id3v2lib/include
                    ${PROJECT_SOURCE_DIR}/lib/libdsdpcm
                    ${PROJECT_SOURCE_DIR}/lib/libdstdec
                    ${PROJECT_SOURCE_DIR}/lib/libdstdec/binding
                    ${PROJECT_SOURCE_DIR}/lib/libdstdec/decoder
                    ${KODI_INCLUDE_DIR}/..
                    ${iconv_INCLUDE_DIRS}
                    ${WAVPACK_INCLUDE_DIRS})

add_subdirectory(lib/libdsdpcm)
add_subdirectory(lib/libdstdec)
add_subdirectory(lib/id3v2lib)

set(SACD_SOURCES src/Addon.cpp
                 src/SACDAudio.cpp
                 src/Settings.cpp
                 src/sacd/id3_tagger.cpp
                 src/sacd/sacd_core.cpp
                 src/sacd/sacd_disc.cpp
                 src/sacd/sacd_dsd.cpp
                 src/sacd/sacd_dsdiff.cpp
                 src/sacd/sacd_dsf.cpp
                 src/sacd/sacd_media.cpp
                 src/sacd/scarletbook.cpp)

set(SACD_HEADERS src/Addon.h
                 src/SACDAudio.h
                 src/Settings.h
                 src/sacd/id3_tagger.h
                 src/sacd/sacd_core.h
                 src/sacd/sacd_disc.h
                 src/sacd/sacd_dsd.h
                 src/sacd/sacd_dsdiff.h
                 src/sacd/sacd_dsf.h
                 src/sacd/sacd_media.h
                 src/sacd/sacd_reader.h
                 src/sacd/scarletbook.h)

set(DEPLIBS dsdpcm dstdec id3v2 ${WAVPACK_LIBRARIES} ${ICONV_LIBRARY})

build_addon(audiodecoder.sacd SACD DEPLIBS)

include(CPack)
