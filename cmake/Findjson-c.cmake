# This module tries to find json-c library and include files
#
# JSONC_INCLUDE_DIRS, path where to find json.h
# JSONC_LIBRARIES, the library to link against
# JSONC_FOUND, If false, do not try to use libWebSockets
#

find_path(JSONC_INCLUDE_DIRS json.h)
find_library(JSONC_LIBRARIES json-c)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(json-c REQUIRED_VARS JSONC_INCLUDE_DIRS JSONC_LIBRARIES)
mark_as_advanced(JSONC_INCLUDE_DIRS JSONC_LIBRARIES)
