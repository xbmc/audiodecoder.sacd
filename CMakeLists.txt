cmake_minimum_required(VERSION 3.5)
project(audiodecoder.sacd)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

find_package(Kodi REQUIRED)

include_directories(${KODI_INCLUDE_DIR}/..)

set(SACD_SOURCES src/Addon.cpp
                 src/SACDAudio.cpp
                 src/Settings.cpp)

set(SACD_HEADERS src/Addon.h
                 src/SACDAudio.h
                 src/Settings.h)

build_addon(audiodecoder.sacd SACD DEPLIBS)

include(CPack)
