set(appstream_template ${CMAKE_SOURCE_DIR}/tools/Linux/kodi.metainfo.xml.in)
set(appstream_po_dir ${CMAKE_SOURCE_DIR}/tools/Linux/translations)
file(GLOB appstream_catalogs CONFIGURE_DEPENDS ${appstream_po_dir}/*.po)

if(appstream_catalogs)
  find_program(MSGFMT_EXECUTABLE NAMES msgfmt REQUIRED)
  set(appstream_build_dir ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/appstream)
  file(MAKE_DIRECTORY ${appstream_build_dir})
  configure_file(${appstream_template}
                 ${appstream_build_dir}/source.metainfo.xml.in COPYONLY)
  set(appstream_languages)
  foreach(catalog IN LISTS appstream_catalogs)
    get_filename_component(language ${catalog} NAME_WE)
    list(APPEND appstream_languages ${language})
    configure_file(${catalog} ${appstream_build_dir}/${language}.po COPYONLY)
  endforeach()
  string(JOIN " " appstream_languages ${appstream_languages})
  file(WRITE ${appstream_build_dir}/LINGUAS "${appstream_languages}\n")
  execute_process(COMMAND ${MSGFMT_EXECUTABLE} --xml
                          --template=${appstream_build_dir}/source.metainfo.xml.in
                          -d ${appstream_build_dir}
                          -o ${appstream_build_dir}/kodi.metainfo.xml.in
                  RESULT_VARIABLE appstream_result
                  ERROR_VARIABLE appstream_error)
  if(NOT appstream_result EQUAL 0)
    message(FATAL_ERROR "Failed to translate AppStream metadata: ${appstream_error}")
  endif()
  set(appstream_template ${appstream_build_dir}/kodi.metainfo.xml.in)
endif()

configure_file(${appstream_template} ${CORE_BUILD_DIR}/${APP_PACKAGE}.metainfo.xml @ONLY)
