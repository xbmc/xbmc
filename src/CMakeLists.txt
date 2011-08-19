
find_package(Boost REQUIRED)

add_definitions(-DTIXML_USE_STL)

set (SOURCES
  FileOps.cpp
  Log.cpp
  ProcessUtils.cpp
  UpdateInstaller.cpp
  UpdateScript.cpp
  UpdaterOptions.cpp
  main.cpp
)

set (HEADERS
  FileOps.h
  Log.h
  ProcessUtils.h
  UpdateInstaller.h
  UpdateScript.h
  UpdaterOptions.h
)

add_executable(updater
  ${SOURCES}
  ${HEADERS}
)

target_link_libraries(updater
  anyoption
  tinyxml
  # TODO - Mac and Windows version
  /usr/lib/libboost_thread.a
)

install(TARGETS updater RUNTIME DESTINATION bin)

