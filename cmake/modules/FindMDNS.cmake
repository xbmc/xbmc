#.rst:
# FindMDNS
# --------
# Finds the mDNS library
#
# This will will define the following variables::
#
# MDNS_FOUND - system has mDNS
# MDNS_INCLUDE_DIRS - the mDNS include directory
# MDNS_LIBRARIES - the mDNS libraries
# MDNS_DEFINITIONS - the mDNS definitions
#
# and the following imported targets::
#
#   MDNS::MDNS   - The mDNSlibrary

find_path(MDNS_INCLUDE_DIR NAMES dmDnsEmbedded.h dns_sd.h)
find_library(MDNS_LIBRARY NAMES mDNSEmbedded dnssd)

find_path(MDNS_EMBEDDED_INCLUDE_DIR NAMES mDnsEmbedded.h)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MDNS
                                  REQUIRED_VARS MDNS_LIBRARY MDNS_INCLUDE_DIR)

if(MDNS_FOUND)
  set(MDNS_INCLUDE_DIRS ${MDNS_INCLUDE_DIR})
  set(MDNS_LIBRARIES ${MDNS_LIBRARY})
  set(MDNS_DEFINITIONS -DHAVE_LIBMDNS=1)
  if(MDNS_EMBEDDED_INCLUDE_DIR)
    list(APPEND MDNS_DEFINITIONS -DHAVE_LIBMDNSEMBEDDED=1)
  endif()

  if(NOT TARGET MDNS::MDNS)
    add_library(MDNS::MDNS UNKNOWN IMPORTED)
    set_target_properties(MDNS::MDNS PROPERTIES
                                     IMPORTED_LOCATION "${MDNS_LIBRARY}"
                                     INTERFACE_INCLUDE_DIRECTORIES "${MDNS_INCLUDE_DIR}"
                                     INTERFACE_COMPILE_DEFINITIONS HAVE_LIBMDNS=1)
    if(MDNS_EMBEDDED_INCLUDE_DIR)
      set_target_properties(MDNS::MDNS PROPERTIES
                                       INTERFACE_COMPILE_DEFINITIONS HAVE_LIBMDNSEMBEDDED=1)
    endif()
  endif()
endif()

mark_as_advanced(MDNS_INCLUDE_DIR MDNS_EMBEDDED_INCLUDE_DIR MDNS_LIBRARY)
