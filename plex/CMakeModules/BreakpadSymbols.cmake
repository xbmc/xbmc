find_program(BZIP2 bzip2 HINTS /usr/bin)

function(GENERATE_DSYMS TGT)
  get_target_property(TARGET_FNAME ${TGT} OUTPUT_NAME)
  add_custom_command(
    OUTPUT "Plex Home Theater.dSYM/Contents/Resources/DWARF/Plex Home Theater"
    COMMAND dsymutil -o "${TARGET_FNAME}.dSYM" "$<TARGET_FILE:${TGT}>"
    DEPENDS ${TGT}
  )
  add_custom_target(${TGT}_dsym ALL DEPENDS "Plex Home Theater.dSYM/Contents/Resources/DWARF/Plex Home Theater")
endfunction(GENERATE_DSYMS APP)

function(GENERATE_BREAKPAD_SYMBOLS APP)
  set(TARGETFILE $<TARGET_FILE:${APP}>)
  get_filename_component(FNAME ${TARGETFILE} NAME_WE)
  set(DEPENDENCY ${APP})
  if(TARGET_OSX)
    # first we need the dsyms file
    GENERATE_DSYMS(${APP})
    get_target_property(TARGET_FNAME ${APP} OUTPUT_NAME)
    set(DEPENDENCY ${APP}_dsym)
    set(TARGETFILE ${TARGET_FNAME}.dSYM)
  endif(TARGET_OSX)

  if(NOT TARGET_WIN32)
    if(TARGET_OSX)
      set(ARCH -${OSX_ARCH})
    endif(TARGET_OSX)
    add_custom_command(
      OUTPUT ${CMAKE_BINARY_DIR}/${APP}-${PLEX_VERSION_STRING}${ARCH}.symbols.bz2
      COMMAND ${PROJECT_SOURCE_DIR}/plex/scripts/dump_syms.sh "${DUMP_SYMS}" "${BZIP2}" "${TARGETFILE}" "${CMAKE_BINARY_DIR}/${APP}-${PLEX_VERSION_STRING}${ARCH}.symbols.bz2"
      DEPENDS ${DEPENDENCY}
    )
	  add_custom_target(${APP}_symbols DEPENDS ${CMAKE_BINARY_DIR}/${APP}-${PLEX_VERSION_STRING}${ARCH}.symbols.bz2)
  else(NOT TARGET_WIN32)
    find_program(SZIP 7za HINTS ${PROJECT_SOURCE_DIR}/project/Win32BuildSetup/tools/7z)
    add_custom_command(
	   OUTPUT ${CMAKE_BINARY_DIR}/${APP}-${PLEX_VERSION_STRING}.symbols.7z
	   COMMAND ${PROJECT_SOURCE_DIR}/plex/scripts/dump_syms.cmd "${DUMP_SYMS}" "${SZIP}" "${TARGETFILE}" "${CMAKE_BINARY_DIR}/${APP}-${PLEX_VERSION_STRING}.symbols"
	   DEPENDS ${DEPENDENCY}
	  )

    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/symserv)
    add_custom_command(
      OUTPUT  ${CMAKE_BINARY_DIR}/PlexHomeTheater-${PLEX_VERSION_STRING}.symsrv.zip
      COMMAND ${PROJECT_SOURCE_DIR}/plex/scripts/create_symstore.cmd "${PROJECT_SOURCE_DIR}" "${PLEX_VERSION_STRING}"
      DEPENDS ${DEPENDENCY} 
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
	  add_custom_target(${APP}_symbols DEPENDS ${CMAKE_BINARY_DIR}/${APP}-${PLEX_VERSION_STRING}.symbols.7z ${CMAKE_BINARY_DIR}/PlexHomeTheater-${PLEX_VERSION_STRING}.symsrv.zip)
  endif(NOT TARGET_WIN32)
endfunction(GENERATE_BREAKPAD_SYMBOLS APP)
