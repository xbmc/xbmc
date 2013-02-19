set(DVDCSS_VERSION 1.2.10)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

plex_find_headers(HEADERS dlfcn.h unistd.h sys/param.h limits.h pwd.h errno.h windows.h sys/ioctl.h linux/cdrom.h IOKit/storage/IODVDMediaBSDClient.h)

set(CAN_BUILD 1)
if(HAVE_SYS_IOCTL_H)
  if(HAVE_LINUX_CDROM_H)
    set(DVD_STRUCT_IN_LINUX_CDROM_H 1 CACHE BOOL "Using Linux DVD Support")
    set(HAVE_LINUX_DVD_STRUCT 1 CACHE BOOL "Have DVD struct under linux")
  elseif(HAVE_IOKIT_STORAGE_IODVDMEDIABSDCLIENT_H)
    set(DARWIN_DVD_IOCTL 1 CACHE BOOL "Using Darwin DVD Support")
  else(HAVE_LINUX_CDROM_H)
    message(STATUS "LIBDVDCSS: No DVD Support, disabling...")
    set(CAN_BUILD 0)
  endif(HAVE_LINUX_CDROM_H)
endif(HAVE_SYS_IOCTL_H)

configure_file(config_plex.h.in config.h)

add_definitions(-D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -DDVDCSS_DIST -DVERSION="${DVDCSS_VERSION}" -D_XBMC)
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${PLEX_LINK_WRAPPED}")

find_all_sources(src dvdcss_SRC)
list(REMOVE_ITEM dvdcss_SRC src/bsdi_ioctl.c src/bsdi_dvd.h)

if(CAN_BUILD)
  add_library(dvdcss STATIC ${dvdcss_SRC})
  
  add_library(dvdcss-${ARCH} MODULE ${dvdcss_SRC})
  target_link_libraries(dvdcss-${ARCH} wrapper)
  
  if(TARGET_COMMON_DARWIN)
    install(TARGETS dvdcss-${ARCH} DESTINATION ${RESOURCEPATH}/system/players/dvdplayer COMPONENT RUNTIME)
  elseif(TARGET_COMMON_LINUX)
    install(TARGETS dvdcss-${ARCH} DESTINATION ${BINPATH}/system/players/dvdplayer COMPONENT RUNTIME)
  elseif(TARGET_WIN32)
    install(TARGETS dvdcss RUNTIME DESTINATION ${RESOURCEPATH}/system/players/dvdplayer LIBRARY DESTINATION ${RESOURCEPATH}/system COMPONENT RUNTIME)
  endif(TARGET_COMMON_DARWIN)
endif(CAN_BUILD)
