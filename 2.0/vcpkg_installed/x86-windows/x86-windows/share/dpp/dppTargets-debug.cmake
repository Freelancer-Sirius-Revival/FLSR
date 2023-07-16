#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "dpp::dpp" for configuration "Debug"
set_property(TARGET dpp::dpp APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(dpp::dpp PROPERTIES
  IMPORTED_IMPLIB_DEBUG "${_IMPORT_PREFIX}/debug/lib/dpp.lib"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/debug/bin/dpp.dll"
  )

list(APPEND _cmake_import_check_targets dpp::dpp )
list(APPEND _cmake_import_check_files_for_dpp::dpp "${_IMPORT_PREFIX}/debug/lib/dpp.lib" "${_IMPORT_PREFIX}/debug/bin/dpp.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
