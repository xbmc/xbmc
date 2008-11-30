# FIXME: python lib needs linux dir
# FIXME: python configure needs linux specifics
# FIXME: python make needs to be run
# FIXME: how do we zip all the files?

SET(SRCS
    XBPython.cpp
    XBPythonDll.cpp
    XBPyThread.cpp
    XBPythonDllFuncs.S)

SET(DIRS xbmcmodule)

# run python's configure
IF("${CMAKE_CURRENT_SOURCE_DIR}/Python/configure" IS_NEWER_THAN
   "${CMAKE_CURRENT_BINARY_DIR}/Python/Makefile")
  IF(APPLE)
    SET(EXTRA_ARGS
          LDFLAGS=-L/opt/local/lib
          "OPT=-I/opt/local/include -Wno-deprecated-declarations"
          MACOSX_DEPLOYMENT_TARGET=10.4)
  ENDIF()
  FILE(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Python")
  EXECUTE_PROCESS(COMMAND
                    "${CMAKE_CURRENT_SOURCE_DIR}/Python/configure"
                    # this is how it's built on Ubuntu
                    --enable-ipv6 --enable-unicode=ucs4 --without-cxx
                    # build a shared library
                    --enable-shared
                    # pymalloc is supposedly not thread safe
                    --without-pymalloc
                    # pass on the user configured CC and CFLAGS
                    CC=${CMAKE_C_COMPILER}
                    CFLAGS=${CMAKE_C_FLAGS}
                    # add any platform dependent arguments
                    ${EXTRA_ARGS}
                  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Python")
ENDIF()

INCLUDE(${CMAKE_SOURCE_DIR}/CMakeLists.common)

