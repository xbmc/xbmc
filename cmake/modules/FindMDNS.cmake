#.rst:
# FindMDNS
# --------
# Finds the mDNS library
#
# This will define the following target:
#
#   MDNS::MDNS   - The mDNSlibrary

if(NOT TARGET MDNS::MDNS)
  find_path(MDNS_INCLUDE_DIR NAMES dmDnsEmbedded.h dns_sd.h
                             NO_CACHE)
  find_library(MDNS_LIBRARY NAMES mDNSEmbedded dnssd
                            NO_CACHE)

  find_path(MDNS_EMBEDDED_INCLUDE_DIR NAMES mDnsEmbedded.h
                                      NO_CACHE)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(MDNS
                                    REQUIRED_VARS MDNS_LIBRARY MDNS_INCLUDE_DIR)

  if(MDNS_FOUND)
    add_library(MDNS::MDNS UNKNOWN IMPORTED)
    set_target_properties(MDNS::MDNS PROPERTIES
                                     IMPORTED_LOCATION "${MDNS_LIBRARY}"
                                     INTERFACE_INCLUDE_DIRECTORIES "${MDNS_INCLUDE_DIR}"
                                     INTERFACE_COMPILE_DEFINITIONS "HAS_MDNS=1;HAS_ZEROCONF=1")
    if(MDNS_EMBEDDED_INCLUDE_DIR)
      set_property(TARGET MDNS::MDNS APPEND PROPERTY
                                            INTERFACE_COMPILE_DEFINITIONS HAS_MDNS_EMBEDDED=1)
    endif()
    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP MDNS::MDNS)
  endif()
endif()
