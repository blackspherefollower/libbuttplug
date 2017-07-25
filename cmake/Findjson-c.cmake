# This module tries to find json-c library and include files
#
# JSONC_INCLUDE_DIRS, path where to find json.h
# JSONC_LIBRARIES, the library to link against
# JSONC_FOUND, If false, do not try to use libWebSockets
#

find_package(PkgConfig)
pkg_check_modules(PC_JSON-C QUIET json-c)

find_path(JSONC_INCLUDE_DIRS json.h HINTS ${PC_JSON-C_INCLUDEDIR} ${PC_JSON-C_INCLUDE_DIRS} PATH_SUFFIXES json-c json)
find_library(JSONC_LIBRARIES NAMES json-c libjson-c HINTS ${PC_JSON-C_LIBDIR} ${PC_JSON-C_LIBRARY_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(json-c REQUIRED_VARS JSONC_INCLUDE_DIRS JSONC_LIBRARIES)
mark_as_advanced(JSONC_INCLUDE_DIRS JSONC_LIBRARIES)
