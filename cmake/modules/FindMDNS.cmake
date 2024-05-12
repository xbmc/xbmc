#.rst:
# FindMDNS
# --------
# Finds the mDNS library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::MDNS   - The mDNSlibrary

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  find_path(MDNS_INCLUDE_DIR NAMES dmDnsEmbedded.h dns_sd.h)
  find_library(MDNS_LIBRARY NAMES mDNSEmbedded dnssd)

  find_path(MDNS_EMBEDDED_INCLUDE_DIR NAMES mDnsEmbedded.h)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(MDNS
                                    REQUIRED_VARS MDNS_LIBRARY MDNS_INCLUDE_DIR)

  if(MDNS_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     IMPORTED_LOCATION "${MDNS_LIBRARY}"
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${MDNS_INCLUDE_DIR}"
                                                                     INTERFACE_COMPILE_DEFINITIONS "HAS_MDNS;HAS_ZEROCONF")
    if(MDNS_EMBEDDED_INCLUDE_DIR)
      set_property(TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} APPEND PROPERTY
                                                                            INTERFACE_COMPILE_DEFINITIONS HAS_MDNS_EMBEDDED)
    endif()
  endif()
endif()
