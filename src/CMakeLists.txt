
add_subdirectory(tests)

find_package(Threads REQUIRED)
include(GenerateCppResourceFile)

if (UNIX)
	add_definitions(-Wall -Wno-non-virtual-dtor -Werror -Wconversion)
endif()

if (UNIX AND NOT APPLE)
	set(LINUX TRUE)
endif()

if (LINUX)
	find_package(GTK2 REQUIRED gtk)
	if(GTK2_FOUND)
		add_definitions(-DHAVE_GTK2)
		include_directories(${GTK2_INCLUDE_DIRS})
		add_library(updatergtk SHARED UpdateDialogGtk.cpp UpdateDialogGtk.h)
		target_link_libraries(updatergtk ${GTK2_LIBRARIES})
	endif(GTK2_FOUND)
endif()

add_definitions(-DTIXML_USE_STL)

set (SOURCES
  AppInfo.cpp
  DirIterator.cpp
  FileUtils.cpp
  Log.cpp
  ProcessUtils.cpp
  StandardDirs.cpp
  UpdateDialog.cpp
  UpdateInstaller.cpp
  UpdateScript.cpp
  UpdaterOptions.cpp
)

if (APPLE)
	set(MAC_DOCK_ICON_CPP_FILE ${CMAKE_CURRENT_BINARY_DIR}/mac_dock_icon.cpp)
	set(MAC_INFO_PLIST_FILE ${CMAKE_CURRENT_BINARY_DIR}/mac_info_plist.cpp)
	generate_cpp_resource_file(resource_macdockicon
	                           ${CMAKE_CURRENT_SOURCE_DIR}/resources
	                           mac.icns ${MAC_DOCK_ICON_CPP_FILE})
	generate_cpp_resource_file(resource_macplist
	                           ${CMAKE_CURRENT_SOURCE_DIR}/resources
	                           Info.plist ${MAC_INFO_PLIST_FILE})
	set(HEADERS ${HEADERS} MacBundle.h)
	set(SOURCES ${SOURCES}
	  MacBundle.cpp
	  StandardDirs.mm
	  StlSymbolsLeopard.cpp
	  UpdateDialogCocoa.mm
	  mac_dock_icon.cpp
	  mac_info_plist.cpp)
endif()

if (WIN32)
	set(SOURCES ${SOURCES} UpdateDialogWin32.cpp)
endif()

set (HEADERS
  AppInfo.h
  DirIterator.h
  FileUtils.h
  Log.h
  ProcessUtils.h
  StandardDirs.h
  UpdateInstaller.h
  UpdateScript.h
  UpdaterOptions.h
)

if (UNIX)
	set(HEADERS ${HEADERS} UpdateDialogAscii.h)
	set(SOURCES ${SOURCES} UpdateDialogAscii.cpp)
endif()

if (LINUX AND GTK2_FOUND)
	# embed the GTK helper library into the updater binary.
	# At runtime it will be extracted and loaded if the
	# GTK libraries are available
	set(GTK_UPDATER_LIB libupdatergtk.so)
	set(GTK_BIN_CPP_FILE ${CMAKE_CURRENT_BINARY_DIR}/libupdatergtk.cpp)
	generate_cpp_resource_file(resource_updatergtk ${CMAKE_CURRENT_BINARY_DIR} ${GTK_UPDATER_LIB} ${GTK_BIN_CPP_FILE})
	add_dependencies(resource_updatergtk updatergtk)

	set(SOURCES ${SOURCES} UpdateDialogGtkFactory.cpp ${GTK_BIN_CPP_FILE})
	set(HEADERS ${HEADERS} UpdateDialogGtkFactory.h)
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

if(LINUX)
  add_dependencies(updatershared resource_updatergtk)
endif()

if(APPLE)
  find_library(COCOA_LIBRARY Cocoa)
  find_library(SECURITY_LIBRARY Security)
  target_link_libraries(updatershared ${SECURITY_LIBRARY} ${COCOA_LIBRARY})
endif()

if (UNIX)
  target_link_libraries(updatershared pthread dl)
endif()

if (WIN32)
	set(EXE_FLAGS WIN32 resources/updater.rc)
endif()

add_executable(updater ${EXE_FLAGS} main.cpp)

target_link_libraries(updater
  updatershared
)

if (SIGN_UPDATER)
	add_custom_command(TARGET updater POST_BUILD COMMAND ${BINARY_SIGNING_TOOL} $<TARGET_FILE:updater>)
endif()

add_executable(zip-tool zip-tool.cpp)
target_link_libraries(zip-tool updatershared)

install(TARGETS updater RUNTIME DESTINATION update_installer)
