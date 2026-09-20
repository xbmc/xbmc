# Determines the Debian architecture name (amd64, arm64, armhf, ...) of the
# build target.
#
# `dpkg --print-architecture` describes the build host, which is only right for
# a native build. A cross build is classified from its GNU target triplet
# instead, because CMAKE_SYSTEM_PROCESSOR alone cannot tell armel from armhf
# (both are "arm" in tools/depends' toolchain).
#
# The following variable is set:
#   <result> - the architecture, or empty if it could not be determined
function(core_debian_architecture result)
  set(arch)
  set(triplet)
  if(CMAKE_CROSSCOMPILING)
    if(CMAKE_LIBRARY_ARCHITECTURE)
      set(triplet ${CMAKE_LIBRARY_ARCHITECTURE})
    elseif(CMAKE_C_COMPILER_TARGET)
      set(triplet ${CMAKE_C_COMPILER_TARGET})
    endif()
    find_program(DPKG_ARCHITECTURE_CMD dpkg-architecture)
    if(triplet AND DPKG_ARCHITECTURE_CMD)
      execute_process(COMMAND ${DPKG_ARCHITECTURE_CMD} -t ${triplet} -qDEB_HOST_ARCH
                      OUTPUT_VARIABLE arch
                      OUTPUT_STRIP_TRAILING_WHITESPACE
                      ERROR_QUIET)
    endif()
  else()
    find_program(DPKG_CMD dpkg)
    if(DPKG_CMD)
      execute_process(COMMAND ${DPKG_CMD} --print-architecture
                      OUTPUT_VARIABLE arch
                      OUTPUT_STRIP_TRAILING_WHITESPACE)
    endif()
  endif()

  if(NOT arch)
    if(triplet MATCHES "gnueabihf$")
      set(arch armhf)
    elseif(triplet MATCHES "gnueabi$")
      set(arch armel)
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64)$")
      set(arch amd64)
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$")
      set(arch arm64)
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(i[3-6]86|x86)$")
      set(arch i386)
    endif()
  endif()
  set(${result} ${arch} PARENT_SCOPE)
endfunction()
