
add_subdirectory(tests)

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

add_library(updatershared
  ${SOURCES}
  ${HEADERS}
)

target_link_libraries(updatershared
  anyoption
  tinyxml
  minizip
  tinythread
  # TODO - Mac and Windows version
  pthread
)

add_executable(updater
  main.cpp
)

target_link_libraries(updater
  updatershared
)

install(TARGETS updater RUNTIME DESTINATION bin)

