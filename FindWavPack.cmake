# - Find WavPack
# Find the native wavpack for windows includes and library
#
#   WAVPACK_FOUND        - True if wavpack for windows found.
#   WAVPACK_INCLUDE_DIRS - where to find wavpack.h, etc.
#   WAVPACK_LIBRARIES    - List of libraries when using wavpack for windows.
#

find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_WAVPACK wavpack QUIET)
endif()

find_path(WAVPACK_INCLUDE_DIRS NAMES wavpack.h
                                     wavpack/wavpack.h
                               PATHS ${PC_WAVPACK_INCLUDEDIR})
find_library(WAVPACK_LIBRARIES NAMES wavpack
                                     libwavpack.lib
                               PATHS ${PC_WAVPACK_LIBDIR})

include("FindPackageHandleStandardArgs")
find_package_handle_standard_args(WavPack REQUIRED_VARS WAVPACK_INCLUDE_DIRS WAVPACK_LIBRARIES)

mark_as_advanced(WAVPACK_INCLUDE_DIRS WAVPACK_LIBRARIES)
