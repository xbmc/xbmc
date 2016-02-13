set(SOURCES libXBMC_addon.cpp)

core_add_library(XBMC_addon NO_MAIN_DEPENDS)
set_target_properties(XBMC_addon PROPERTIES POSITION_INDEPENDENT_CODE True)
target_include_directories(XBMC_addon PRIVATE
                           ${CMAKE_CURRENT_SOURCE_DIR}
                           ${CORE_SOURCE_DIR}/xbmc/addons/include
                           ${CORE_SOURCE_DIR}/xbmc)
