cmake_minimum_required(VERSION 3.16)
project(boost)

include(ExternalProject)

find_package(Python QUIET COMPONENTS Interpreter)

# The list of boost libraries to build
list(APPEND BOOST_COMPONENTS "container")
list(APPEND BOOST_COMPONENTS "json")
list(APPEND BOOST_COMPONENTS "system")

# Boost build options
list(APPEND BOOST_OPTIONS "link=static")
list(APPEND BOOST_OPTIONS "threading=multi")
list(APPEND BOOST_OPTIONS "cxxstd=17")

if(WIN32)
  list(APPEND BOOST_OPTIONS "--prefix=${CMAKE_CURRENT_BINARY_DIR}/boost-prefix")
  list(APPEND BOOST_OPTIONS "cflags=${CMAKE_C_FLAGS} /FS")
  list(APPEND BOOST_OPTIONS "cxxflags=${CMAKE_CXX_FLAGS} /FS")
  list(APPEND BOOST_OPTIONS "debug-store=database")
  list(APPEND BOOST_OPTIONS "linkflags=${CMAKE_STATIC_LINKER_FLAGS} /FS")
  list(APPEND BOOST_OPTIONS "pch=off")
  list(APPEND BOOST_OPTIONS "toolset=msvc")
  foreach(BOOST_COMPONENT ${BOOST_COMPONENTS})
    list(APPEND BOOST_OPTIONS "--with-${BOOST_COMPONENT}")
  endforeach()
else()
  list(APPEND BOOST_OPTIONS "cflags=${CMAKE_C_FLAGS}")
  list(APPEND BOOST_OPTIONS "cxxflags=${CMAKE_CXX_FLAGS}")
  list(APPEND BOOST_OPTIONS "linkflags=${CMAKE_STATIC_LINKER_FLAGS}")
  list(APPEND BOOST_OPTIONS "toolset=gcc-cross")
endif()

if(CMAKE_BUILD_TYPE STREQUAL Debug)
  list(APPEND BOOST_OPTIONS "variant=debug")
else()
  list(APPEND BOOST_OPTIONS "variant=release")
endif()

string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" CPU_LC)

if(WIN32)
  if("${CMAKE_GENERATOR_PLATFORM}" STREQUAL "Win32")
    list(APPEND BOOST_OPTIONS "architecture=x86" "address-model=32")
  elseif("${CMAKE_GENERATOR_PLATFORM}" STREQUAL "x64")
    list(APPEND BOOST_OPTIONS "architecture=x86" "address-model=64")
  elseif("${CMAKE_GENERATOR_PLATFORM}" STREQUAL "ARM")
    list(APPEND BOOST_OPTIONS "architecture=arm" "address-model=64")
  else()
    message(FATAL_ERROR "CMAKE_GENERATOR_PLATFORM not known: ${CMAKE_GENERATOR_PLATFORM}")
  endif()
else()
  if(CPU_LC MATCHES "arm64" OR CPU_LC MATCHES "aarch64")
    list(APPEND BOOST_OPTIONS "architecture=arm" "address-model=64")
  elseif(CPU_LC MATCHES "arm")
    list(APPEND BOOST_OPTIONS "architecture=arm" "address-model=32")
  elseif(CPU_LC MATCHES "x86_64" OR CPU_LC MATCHES "amd64")
    list(APPEND BOOST_OPTIONS "architecture=x86" "address-model=64")
  elseif(CPU_LC MATCHES "i.86")
    list(APPEND BOOST_OPTIONS "architecture=x86" "address-model=32")
  elseif(CPU_LC MATCHES "riscv64")
    list(APPEND BOOST_OPTIONS "architecture=parisc" "address-model=64")
  elseif(CPU_LC MATCHES "powerpc")
    list(APPEND BOOST_OPTIONS "architecture=power" "address-model=64")
  else()
    message(FATAL_ERROR "CMAKE_SYSTEM_PROCESSOR not known: ${CMAKE_SYSTEM_PROCESSOR}")
  endif()
endif()

# TODO: FreeBSD (verify), iOS, tvOS
if(CMAKE_SYSTEM_NAME STREQUAL "freebsd")
  list(APPEND BOOST_OPTIONS "target-os=freebsd")
elseif(CMAKE_SYSTEM_NAME STREQUAL "linux")
  list(APPEND BOOST_OPTIONS "target-os=linux")
elseif(CORE_SYSTEM_NAME STREQUAL "android")
  list(APPEND BOOST_OPTIONS "target-os=android")
elseif(CORE_SYSTEM_NAME STREQUAL "osx")
  list(APPEND BOOST_OPTIONS "target-os=darwin")
endif()

string(REPLACE ";" "," BOOST_COMPONENTS_LIST "${BOOST_COMPONENTS}")
string(REPLACE ";" " " BOOST_OPTIONS_LIST "${BOOST_OPTIONS}")

# Boost configuration command
if(WIN32)
  set(CONFIGURE_COMMAND <SOURCE_DIR>/bootstrap.bat)
  set(INSTALL_COMMAND
    ${CMAKE_COMMAND}
      "-DBOOST_COMPONENTS_LIST=${BOOST_COMPONENTS_LIST}"
      "-DBOOST_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/boost-prefix"
      "-DCMAKE_STATIC_LIBRARY_SUFFIX=${CMAKE_STATIC_LIBRARY_SUFFIX}"
      -P rename_boost_libraries.cmake)
else()
  set(CONFIGURE_COMMAND <SOURCE_DIR>/bootstrap.sh
                        --prefix=<INSTALL_DIR>
                        --with-python=${PYTHON_EXECUTABLE}
                        --with-libraries=${BOOST_COMPONENTS_LIST}
                        --without-icu &&
                        echo "using gcc : cross : ${CMAKE_CXX_COMPILER_LAUNCHER} ${CMAKE_CXX_COMPILER} $<SEMICOLON>" >>
                          <SOURCE_DIR>/project-config.jam)
  set(INSTALL_COMMAND ${CMAKE_COMMAND} -E true)
endif()

# Add the boost project
ExternalProject_Add(boost
  PREFIX boost-prefix
  SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
  BUILD_IN_SOURCE 1
  CONFIGURE_COMMAND ${CONFIGURE_COMMAND}
  BUILD_COMMAND <SOURCE_DIR>/b2 ${BOOST_OPTIONS} install
  INSTALL_COMMAND ${INSTALL_COMMAND})

# TODO: Better way to handle subdirectory on Windows?
if(WIN32)
  install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/boost-prefix/include/boost-1_83"
          DESTINATION "include")
  install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/boost-prefix/include/boost-1_83/boost"
          DESTINATION "include")
else()
  install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/boost-prefix/include/boost"
          DESTINATION "include")
endif()

foreach(BOOST_COMPONENT ${BOOST_COMPONENTS})
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/boost-prefix/lib/libboost_${BOOST_COMPONENT}${CMAKE_STATIC_LIBRARY_SUFFIX}"
          DESTINATION "lib")
endforeach()
