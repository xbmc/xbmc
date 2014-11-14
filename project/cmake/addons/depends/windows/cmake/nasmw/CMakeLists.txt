project(nasmw)

cmake_minimum_required(VERSION 2.8)

include(ExternalProject)

externalproject_add(nasmw
                    SOURCE_DIR ${PROJECT_SOURCE_DIR}
                    BINARY_DIR ${PROJECT_SOURCE_DIR}
                    INSTALL_DIR ${PROJECT_SOURCE_DIR}
                    CONFIGURE_COMMAND ""
                    BUILD_COMMAND nmake /f Mkfiles/msvc.mak
                    INSTALL_COMMAND "")

install(PROGRAMS nasm.exe DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
                          RENAME nasmw.exe)
