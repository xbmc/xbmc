
find_package(Boost REQUIRED)
find_package(Threads REQUIRED)

add_definitions(-DTIXML_USE_STL)

set (SOURCES
  Dir.cpp
  FileOps.cpp
  Log.cpp
  ProcessUtils.cpp
  UpdateInstaller.cpp
  UpdateScript.cpp
  UpdaterOptions.cpp
  main.cpp
)

set (HEADERS
  Dir.h
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
  minizip
  # TODO - Mac and Windows version
  pthread
  /usr/lib/libboost_thread.a
)

install(TARGETS updater RUNTIME DESTINATION bin)

