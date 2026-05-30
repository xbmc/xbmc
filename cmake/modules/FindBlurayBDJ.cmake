#.rst:
# FindBlurayBDJ
# ----------
# Finds the BD-J library 
#
# Variable:
#   BDJ_HOME  - directory to search for libbluray BDJ jar files
#
# This will define the following target:
#
#   Extras::BlurayBDJ   - The libbluray BD-J jar files

if(NOT TARGET Extras::${CMAKE_FIND_PACKAGE_NAME})

  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC libbluraybdj)

  get_property(build_jar TARGET ${APP_NAME_LC}::Bluray PROPERTY BDJ_BUILD SET)

  if(build_jar)
    # Libbluray is building the bdj components, so set paths on expected output
    set(${CMAKE_FIND_PACKAGE_NAME}_FOUND ON)
    get_property(bdj_version TARGET ${APP_NAME_LC}::Bluray PROPERTY BDJ_VERSION)

    set(bdj_jar "${DEPENDS_PATH}/share/java/libbluray-j2se-${bdj_version}.jar")
    set(bdj_awt_jar "${DEPENDS_PATH}/share/java/libbluray-awt-j2se-${bdj_version}.jar")
  else()
    if(BDJ_HOME)
      set(BD_SEARCH_PATHS ${BDJ_HOME})
    endif()

    list(APPEND BD_SEARCH_PATHS ${DEPENDS_PATH}/share/java
                                ${DEPENDS_PATH}/bin
                                /usr/share/java)

    if(NOT ${CMAKE_FIND_PACKAGE_NAME}_FIND_QUIETLY)
      message(CHECK_START "Looking for Bluray-BDJ jars")
      list(APPEND CMAKE_MESSAGE_INDENT "  ")

      message(CHECK_START "Finding libbluray AWT jar")
    endif()

    foreach(bdj_path IN LISTS BD_SEARCH_PATHS)
      file(GLOB bdj_awt_jar LIST_DIRECTORIES false ${bdj_path}/libbluray-awt-j2[sm]e-*.jar)

      if(EXISTS "${bdj_awt_jar}")
        if(NOT ${CMAKE_FIND_PACKAGE_NAME}_FIND_QUIETLY)
          message(CHECK_PASS "found")
        endif()

        break()
      endif()
    endforeach()

    if(NOT bdj_awt_jar)
      if(NOT ${CMAKE_FIND_PACKAGE_NAME}_FIND_QUIETLY)
        message(CHECK_FAIL "not found")
      endif()

      list(APPEND missingComponents "libbluray-awt-[j2se|j2me]-*.jar")
    endif()

    if(NOT ${CMAKE_FIND_PACKAGE_NAME}_FIND_QUIETLY)
      message(CHECK_START "Finding libbluray jar")
    endif()

    foreach(bdj_path IN LISTS BD_SEARCH_PATHS)
      file(GLOB bdj_jar LIST_DIRECTORIES false ${bdj_path}/libbluray-j2[sm]e-*.jar)

      if(EXISTS "${bdj_jar}")
        if(NOT ${CMAKE_FIND_PACKAGE_NAME}_FIND_QUIETLY)
          message(CHECK_PASS "found")
        endif()

        break()
      endif()
    endforeach()

    if(NOT bdj_jar)
      if(NOT ${CMAKE_FIND_PACKAGE_NAME}_FIND_QUIETLY)
        message(CHECK_FAIL "not found")
      endif()

      list(APPEND missingComponents "libbluray-[j2se|j2me]-*.jar")
    endif()

    if(NOT ${CMAKE_FIND_PACKAGE_NAME}_FIND_QUIETLY)
      list(POP_BACK CMAKE_MESSAGE_INDENT)
    endif()

    if(missingComponents)
      if(NOT ${CMAKE_FIND_PACKAGE_NAME}_FIND_QUIETLY)
        message(CHECK_FAIL "missing components: ${missingComponents}")
      endif()
    else()
      if(NOT ${CMAKE_FIND_PACKAGE_NAME}_FIND_QUIETLY)
        message(CHECK_PASS "all components found")
      endif()

      set(${CMAKE_FIND_PACKAGE_NAME}_FOUND ON)
    endif()
  endif()

  if(${CMAKE_FIND_PACKAGE_NAME}_FOUND)
    add_library(Extras::${CMAKE_FIND_PACKAGE_NAME} INTERFACE IMPORTED)
    set_target_properties(Extras::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                             INTERFACE_LINK_LIBRARIES "${bdj_jar};${bdj_awt_jar}")
  endif()
endif()
