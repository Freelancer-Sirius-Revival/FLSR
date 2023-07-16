
####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was dppConfig.cmake.in                            ########

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../" ABSOLUTE)

macro(set_and_check _var _file)
  set(${_var} "${_file}")
  if(NOT EXISTS "${_file}")
    message(FATAL_ERROR "File or directory ${_file} referenced by variable ${_var} does not exist !")
  endif()
endmacro()

macro(check_required_components _NAME)
  foreach(comp ${${_NAME}_FIND_COMPONENTS})
    if(NOT ${_NAME}_${comp}_FOUND)
      if(${_NAME}_FIND_REQUIRED_${comp})
        set(${_NAME}_FOUND FALSE)
      endif()
    endif()
  endforeach()
endmacro()

####################################################################################

set_and_check(EXPORT_TARGETS_FILE_NEW "${PACKAGE_PREFIX_DIR}/share/dpp/dppTargets.cmake")

include("${EXPORT_TARGETS_FILE_NEW}")

if (WIN32)
	if (EXISTS "${PACKAGE_PREFIX_DIR}/bin/dpp.pdb")
		set_and_check(RELEASE_PDB_FILE_PATH "${PACKAGE_PREFIX_DIR}/bin/dpp.pdb")
		cmake_path(GET RELEASE_PDB_FILE_PATH FILENAME RELEASE_PDB_FILE_NAME)
	endif()	
	if (EXISTS "${PACKAGE_PREFIX_DIR}/debug/bin/dpp.pdb")
		set_and_check(DEBUG_PDB_FILE_PATH "${PACKAGE_PREFIX_DIR}/debug/bin/dpp.pdb")
		cmake_path(GET DEBUG_PDB_FILE_PATH FILENAME DEBUG_PDB_FILE_NAME)
	endif()	
endif()

check_required_components("dpp")
