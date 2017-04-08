file(STRINGS ${CMAKE_SOURCE_DIR}/xbmc/interfaces/json-rpc/schema/version.txt jsonrpc_version)

execute_process(COMMAND ${CMAKE_COMMAND} -E remove ${CORE_BINARY_DIR}/addons/xbmc.json/addon.xml)
configure_file(${CMAKE_SOURCE_DIR}/addons/xbmc.json/addon.xml.in
               ${CORE_BINARY_DIR}/addons/xbmc.json/addon.xml @ONLY)
