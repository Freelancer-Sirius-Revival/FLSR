#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "SQLiteCpp" for configuration "Debug"
set_property(TARGET SQLiteCpp APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(SQLiteCpp PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/debug/lib/SQLiteCpp.lib"
  )

list(APPEND _cmake_import_check_targets SQLiteCpp )
list(APPEND _cmake_import_check_files_for_SQLiteCpp "${_IMPORT_PREFIX}/debug/lib/SQLiteCpp.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
