# - Find TagLib
# Find the native taglib for windows includes and library
#
#   TAGLIB_FOUND        - True if taglib for windows found.
#   TAGLIB_INCLUDE_DIRS - where to find taglib.h, etc.
#   TAGLIB_LIBRARIES    - List of libraries when using taglib for windows.
#

find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_TAGLIB taglib QUIET)
endif()

find_path(TAGLIB_INCLUDE_DIRS NAMES taglib/taglib.h
                               PATHS ${PC_TAGLIB_INCLUDEDIR})
find_library(TAGLIB_LIBRARIES NAMES tag
                               PATHS ${PC_TAGLIB_LIBDIR})

include("FindPackageHandleStandardArgs")
find_package_handle_standard_args(TagLib REQUIRED_VARS TAGLIB_INCLUDE_DIRS TAGLIB_LIBRARIES)

mark_as_advanced(TAGLIB_INCLUDE_DIRS TAGLIB_LIBRARIES)
