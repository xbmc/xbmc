SET(SRCS DllLoader.cpp DllLoaderContainer.cpp dll.cpp coff.cpp dll_tracker.cpp dll_tracker_library.cpp dll_tracker_file.cpp dll_tracker_memory.cpp dll_tracker_critical_section.cpp dll_util.cpp SoLoader.cpp LibraryLoader.cpp ldt_keeper.c mmap_anon.c)

SET(DIRS exports)

ADD_DEFINITIONS(-DAPI_DEBUG)

INCLUDE(${CMAKE_SOURCE_DIR}/CMakeLists.common)

