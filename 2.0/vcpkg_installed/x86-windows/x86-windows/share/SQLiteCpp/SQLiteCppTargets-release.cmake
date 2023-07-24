#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "SQLiteCpp" for configuration "Release"
set_property(TARGET SQLiteCpp APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(SQLiteCpp PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/SQLiteCpp.lib"
  IMPORTED_LINK_DEPENDENT_LIBRARIES_RELEASE "unofficial::sqlite3::sqlite3"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/SQLiteCpp.dll"
  )

list(APPEND _cmake_import_check_targets SQLiteCpp )
list(APPEND _cmake_import_check_files_for_SQLiteCpp "${_IMPORT_PREFIX}/lib/SQLiteCpp.lib" "${_IMPORT_PREFIX}/bin/SQLiteCpp.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
