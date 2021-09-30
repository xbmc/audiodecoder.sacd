cmake_minimum_required(VERSION 3.5)
project(libdsdpcm)

set(CMAKE_CXX_STANDARD 17)

# Include directory paths
include_directories(${CMAKE_CURRENT_SOURCE_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR}/binding
                    ${CMAKE_CURRENT_SOURCE_DIR}/decoder)

set(SOURCES DSDPCMConverterEngine.cpp
            Fir_IPP.cpp)

set(HEADERS DSDPCMConstants.h
            DSDPCMConverterDirect.h
            DSDPCMConverterEngine.h
            DSDPCMConverter.h
            DSDPCMConverterMultistage.h
            DSDPCMFilterSetup.h
            DSDPCMFir.h
            DSDPCMFir_IPP.h
            DSDPCMUtil.h
            Fir_IPP.h
            PCMPCMFir.h
            PCMPCMFir_IPP.h
            semaphore.h)

add_library(dsdpcm STATIC ${SOURCES} ${HEADERS})
set_property(TARGET dsdpcm PROPERTY POSITION_INDEPENDENT_CODE ON)
