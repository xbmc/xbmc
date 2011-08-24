
add_subdirectory(tests)

find_package(Threads REQUIRED)

if (UNIX)
	add_definitions(-Wall -Werror -Wconversion)
endif()

include(UsePkgConfig)
pkgconfig(gtk+-2.0 GTK2_INCLUDE_DIR GTK2_LINK_DIR GTK2_LINK_FLAGS GTK2_CFLAGS)

option(ENABLE_GTK ON)

if (ENABLE_GTK)
	include_directories(${GTK2_INCLUDE_DIR})
	add_definitions(${GTK2_CFLAGS})
	add_definitions(-DENABLE_GTK)
endif()

add_definitions(-DTIXML_USE_STL)

set (SOURCES
  DirIterator.cpp
  FileOps.cpp
  Log.cpp
  ProcessUtils.cpp
  UpdateInstaller.cpp
  UpdateScript.cpp
  UpdaterOptions.cpp
)

if (ENABLE_GTK)
	set(SOURCES ${SOURCES} UpdateDialogGtk.cpp)
endif()
if (APPLE)
	set(SOURCES ${SOURCES} UpdateDialogCocoa.mm)
endif()
if (WIN32)
	set(SOURCES ${SOURCES} UpdateDialogWin32.cpp)
endif()

set (HEADERS
  DirIterator.h
  FileOps.h
  Log.h
  ProcessUtils.h
  UpdateInstaller.h
  UpdateScript.h
  UpdaterOptions.h
)

if (ENABLE_GTK)
	set(HEADERS ${HEADERS} UpdateDialogGtk.h)
endif()
if (APPLE)
	set(HEADERS ${HEADERS} UpdateDialogCocoa.h)
endif()
if (WIN32)
    set(HEADERS ${HEADERS} UpdateDialogWin32.h)
endif()

add_library(updatershared
  ${SOURCES}
  ${HEADERS}
)

target_link_libraries(updatershared
  anyoption
  tinyxml
  minizip
  tinythread
  ${GTK2_LINK_FLAGS}
)

if (ENABLE_GTK)
	target_link_libraries(updatershared
	${GTK2_LINK_FLAGS})
endif()

if (UNIX)
  target_link_libraries(updatershared pthread)
endif()

if (WIN32)
	set(EXE_FLAGS WIN32)
endif()

add_executable(updater ${EXE_FLAGS} main.cpp)

if(APPLE)
	set_target_properties(
		updater
		PROPERTIES LINK_FLAGS "-framework Security -framework Cocoa"
	)
endif()

target_link_libraries(updater
  updatershared
)

install(TARGETS updater RUNTIME DESTINATION bin)

