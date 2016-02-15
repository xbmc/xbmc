set(SOURCES libXBMC_codec.cpp)

core_add_library(XBMC_codec NO_MAIN_DEPENDS)
set_target_properties(XBMC_codec PROPERTIES POSITION_INDEPENDENT_CODE True)
target_include_directories(XBMC_codec PRIVATE
                           ${CMAKE_CURRENT_SOURCE_DIR}
                           ${CORE_SOURCE_DIR}/xbmc/addons/include
                           ${CORE_SOURCE_DIR}/xbmc)
