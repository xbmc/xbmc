
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
	add_library(updatergtk SHARED UpdateDialogGtk.cpp UpdateDialogGtk.h)
	target_link_libraries(updatergtk ${GTK2_LINK_FLAGS})
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

if (APPLE)
	set(SOURCES ${SOURCES} UpdateDialogCocoa.mm)
endif()
if (WIN32)
	set(SOURCES ${SOURCES} UpdateDialogWin32.cpp)
endif()

set (HEADERS
  AppInfo.h
  DirIterator.h
  FileOps.h
  Log.h
  ProcessUtils.h
  UpdateInstaller.h
  UpdateScript.h
  UpdaterOptions.h
)

if (UNIX)
	set(HEADERS ${HEADERS} UpdateDialogAscii.h)
	set(SOURCES ${SOURCES} UpdateDialogAscii.cpp)
endif()

if (ENABLE_GTK)
	# embed the GTK helper library into the updater binary.
	# At runtime it will be extracted and loaded if the
	# GTK libraries are available

	set(GTK_UPDATER_LIB libupdatergtk.so)
	set(GTK_BIN_FILE ${CMAKE_CURRENT_BINARY_DIR}/libupdatergtk.cpp)
	add_custom_command(OUTPUT ${GTK_BIN_FILE}
	                   COMMAND xxd -i ${GTK_UPDATER_LIB} ${GTK_BIN_FILE}
	                   DEPENDS updatergtk)
	set(SOURCES ${SOURCES} UpdateDialogGtkWrapper.cpp ${GTK_BIN_FILE})
	set(HEADERS ${HEADERS} UpdateDialogGtkWrapper.h)
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
)

if (UNIX)
  target_link_libraries(updatershared pthread dl)
endif()

if (WIN32)
	set(EXE_FLAGS WIN32 resources/updater.rc)
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

