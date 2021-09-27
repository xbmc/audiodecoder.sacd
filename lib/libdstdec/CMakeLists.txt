cmake_minimum_required(VERSION 3.5)
project(libdstdec)

set(CMAKE_CXX_STANDARD 17)

# Include directory paths
include_directories(${CMAKE_CURRENT_SOURCE_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR}/binding
                    ${CMAKE_CURRENT_SOURCE_DIR}/decoder)

set(SOURCES binding/dst_decoder_mt.cpp
            decoder/decoder.cpp)

set(HEADERS binding/dst_decoder_mt.h
            binding/semaphore.h
            decoder/decoder.h
            ac.h
            common.h
            consts.h
            ct.h
            fh.h
            fr.h
            segment.h
            stream.h)

add_library(dstdec STATIC ${SOURCES} ${HEADERS})
set_property(TARGET dstdec PROPERTY POSITION_INDEPENDENT_CODE ON)
