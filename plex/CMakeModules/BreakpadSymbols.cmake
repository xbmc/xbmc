find_program(BZIP2 bzip2 HINTS /usr/bin)

function(GENERATE_DSYMS TGT)
  add_custom_command(
    OUTPUT ${TGT}.dSYM
    COMMAND dsymutil -f -o "${TGT}.dSYM" "$<TARGET_FILE:${TGT}>"
    DEPENDS ${TGT}
  )
  add_custom_target(${TGT}_dsym ALL DEPENDS ${TGT}.dSYM)
endfunction(GENERATE_DSYMS APP)

function(GENERATE_BREAKPAD_SYMBOLS APP)
  set(TARGETFILE $<TARGET_FILE:${APP}>)
  get_filename_component(FNAME ${TARGETFILE} NAME_WE)
  set(DEPENDENCY ${APP})
  if(APPLE)
    # first we need the dsyms file
    GENERATE_DSYMS(${APP})
    set(DEPENDENCY ${APP}_dsym)
    set(TARGETFILE ${APP}.dSYM)
  endif(APPLE)

  if(NOT WIN32)
    add_custom_command(
      OUTPUT ${CMAKE_BINARY_DIR}/${APP}-${PLEX_VERSION_STRING}.symbols.bz2
      COMMAND ${PROJECT_SOURCE_DIR}/plex/scripts/dump_syms.sh "${DUMP_SYMS}" "${BZIP2}" "${TARGETFILE}" "${CMAKE_BINARY_DIR}/${APP}-${PLEX_VERSION_STRING}.symbols.bz2"
      DEPENDS ${DEPENDENCY}
    )
	add_custom_target(${APP}_symbols DEPENDS ${CMAKE_BINARY_DIR}/${APP}-${PLEX_VERSION_STRING}.symbols.bz2)
  else(NOT WIN32)
    find_program(SZIP 7za HINTS ${PROJECT_SOURCE_DIR}/project/Win32BuildSetup/tools/7z)
    add_custom_command(
	  OUTPUT ${CMAKE_BINARY_DIR}/${APP}-${PLEX_VERSION_STRING}.symbols.7z
	  COMMAND ${PROJECT_SOURCE_DIR}/plex/scripts/dump_syms.cmd "${DUMP_SYMS}" "${SZIP}" "${TARGETFILE}" "${CMAKE_BINARY_DIR}/${APP}-${PLEX_VERSION_STRING}.symbols"
	  DEPENDS ${DEPENDENCY}
	)
	add_custom_target(${APP}_symbols DEPENDS ${CMAKE_BINARY_DIR}/${APP}-${PLEX_VERSION_STRING}.symbols.7z)
  endif(NOT WIN32)
endfunction(GENERATE_BREAKPAD_SYMBOLS APP)
