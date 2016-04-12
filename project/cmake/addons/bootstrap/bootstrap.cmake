list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

# make sure that the installation location has been specified
if(NOT CMAKE_INSTALL_PREFIX)
  message(FATAL_ERROR "CMAKE_INSTALL_PREFIX has not been specified")
endif()

# figure out which addons to bootstrap (defaults to all)
if(NOT ADDONS_TO_BUILD)
  set(ADDONS_TO_BUILD "all")
else()
  string(STRIP "${ADDONS_TO_BUILD}" ADDONS_TO_BUILD)
  message(STATUS "Bootstrapping following addons: ${ADDONS_TO_BUILD}")
  separate_arguments(ADDONS_TO_BUILD)
endif()

# find all addon definitions and go through them
file(GLOB_RECURSE ADDON_DEFINITIONS ${PROJECT_SOURCE_DIR}/*.txt)
foreach(ADDON_DEFINITION_FILE ${ADDON_DEFINITIONS})
  # ignore platforms.txt
  if(NOT (ADDON_DEFINITION_FILE MATCHES platforms.txt))
    # read the addon definition file
    file(STRINGS ${ADDON_DEFINITION_FILE} ADDON_DEFINITION)
    separate_arguments(ADDON_DEFINITION)

    # extract the addon definition's identifier
    list(GET ADDON_DEFINITION 0 ADDON_ID)

    # check if the addon definition should be built
    list(FIND ADDONS_TO_BUILD ${ADDON_ID} ADDONS_TO_BUILD_IDX)
    if(ADDONS_TO_BUILD_IDX GREATER -1 OR "${ADDONS_TO_BUILD}" STREQUAL "all")
      # get the path to the addon definition directory
      get_filename_component(ADDON_DEFINITION_DIR ${ADDON_DEFINITION_FILE} PATH)

      # install the addon definition
      message(STATUS "Bootstrapping ${ADDON_ID} addon...")
      file(INSTALL ${ADDON_DEFINITION_DIR} DESTINATION ${CMAKE_INSTALL_PREFIX})
    endif()
  endif()
endforeach()
