include(${CORE_SOURCE_DIR}/cmake/scripts/common/Macros.cmake)

core_find_versions()

# add-on xml's
file(GLOB ADDON_XML_IN_FILE ${CORE_SOURCE_DIR}/addons/*/addon.xml.in)

# remove 'xbmc.json', will be created from 'xbmc/interfaces/json-rpc/schema/CMakeLists.txt'
list(REMOVE_ITEM ADDON_XML_IN_FILE ${CORE_SOURCE_DIR}/addons/xbmc.json/addon.xml.in)

foreach(loop_var ${ADDON_XML_IN_FILE})
  list(GET loop_var 0 xml_name)

  string(REPLACE "/addon.xml.in" "" source_dir ${xml_name})

  string(REPLACE ${CORE_SOURCE_DIR} ${BUNDLEDIR} dest_dir ${source_dir})

  file(MAKE_DIRECTORY ${dest_dir})
  configure_file(${source_dir}/addon.xml.in ${dest_dir}/addon.xml @ONLY)

  unset(source_dir)
  unset(dest_dir)
  unset(xml_name)
endforeach()
