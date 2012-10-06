# CPLUFF

# XBMC expects it to be called libcpluff...
set(CMAKE_SHARED_MODULE_PREFIX "lib")

set(PACKAGE_VERSION "0.1.3")
set(CP_VERSION_MAJOR "0")
set(CP_VERSION_MINOR "1")
set(CP_VERSION_REV "3")

include(CheckIncludeFiles)
include(CheckFunctionExists)
include(CheckSymbolExists)
include(CheckLibraryExists)

if(APPLE)
  set(HAVE_CFLOCALECOPYCURRENT 1)
  set(HAVE_CFPREFERENCESCOPYAPPVALUE 1)
endif()

check_include_files(memory.h HAVE_MEMORY_H)
check_include_files(stdint.h HAVE_STDINT_H)
check_include_files(dlfcn.h HAVE_DLFCN_H)
check_function_exists(gettext HAVE_DCGETTEXT)
check_function_exists(dcgettext HAVE_DCGETTEXT)
check_function_exists(iconv HAVE_ICONV)
check_function_exists(stat HAVE_STAT)

set(CP_ABI_COMPATIBILITY "\"0.1\"")

# TODO: set these correctly for windows
if(UNIX)
  set(CP_FNAMESEP_CHAR "'/'")
  set(CP_FNAMESEP_STR "\"/\"")
  set(CP_THREADS "\"Posix\"")
  set(CP_SHREXT "\"`test .$module = .yes && echo .so || echo .dylib`\"")
  set(DLOPEN_POSIX 1)
  if(APPLE)
    set(CP_HOST "darwin")
  else()
    set(CP_HOST "linux")
  endif()
endif()

add_definitions(-DCP_C_API=CP_EXPORT -DCP_HOST="${CP_HOST}" -DHAVE_CONFIG_H)
include_directories(. libcpluff)

configure_file(libcpluff/cpluffdef.h.in ${root}/lib/cpluff/libcpluff/cpluffdef.h)
configure_file(config_plex.h.in ${root}/lib/cpluff/config.h)

set(libcpluff_srcs
  libcpluff/psymbol.c
  libcpluff/pscan.c
  libcpluff/ploader.c
  libcpluff/pinfo.c
  libcpluff/pcontrol.c
  libcpluff/serial.c
  libcpluff/logging.c
  libcpluff/context.c
  libcpluff/cpluff.c 
  libcpluff/util.c 
  kazlib/list.c
  kazlib/list.h
  kazlib/hash.c
  kazlib/hash.h
  libcpluff/internal.h
  libcpluff/thread.h
  libcpluff/util.h
  libcpluff/defines.h
  libcpluff/thread_posix.c
)
add_library(cpluff-${ARCH} MODULE ${libcpluff_srcs})
install(TARGETS cpluff-${ARCH} DESTINATION Plex.app/Contents/Resources/XBMC/system)