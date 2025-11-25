# Voice input module for semantic search

set(SOURCES
  VoiceInputManager.cpp
  VoiceSearchIntegration.cpp
)

set(HEADERS
  IVoiceInput.h
  VoiceInputManager.h
  VoiceSearchIntegration.h
)

# Platform-specific implementations
if(TARGET_WINDOWS)
  list(APPEND SOURCES platform/WindowsVoiceInput.cpp)
  list(APPEND HEADERS platform/WindowsVoiceInput.h)
  # Windows SAPI libraries
  list(APPEND DEPLIBS sapi.lib)
elseif(TARGET_ANDROID)
  list(APPEND SOURCES platform/AndroidVoiceInput.cpp)
  list(APPEND HEADERS platform/AndroidVoiceInput.h)
elseif(TARGET_DARWIN)
  list(APPEND SOURCES platform/DarwinVoiceInput.cpp)
  list(APPEND HEADERS platform/DarwinVoiceInput.h)
  # Darwin frameworks
  if(TARGET_DARWIN_IOS OR TARGET_DARWIN_TVOS)
    list(APPEND DEPLIBS "-framework Speech" "-framework AVFoundation")
  else()
    list(APPEND DEPLIBS "-framework AppKit" "-framework Foundation")
  endif()
elseif(TARGET_LINUX)
  list(APPEND SOURCES platform/LinuxVoiceInput.cpp)
  list(APPEND HEADERS platform/LinuxVoiceInput.h)
  # PocketSphinx support (optional)
  find_package(PkgConfig)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(POCKETSPHINX pocketsphinx)
    pkg_check_modules(SPHINXBASE sphinxbase)
    if(POCKETSPHINX_FOUND AND SPHINXBASE_FOUND)
      set(HAS_POCKETSPHINX 1)
      list(APPEND DEPLIBS ${POCKETSPHINX_LIBRARIES} ${SPHINXBASE_LIBRARIES})
      include_directories(${POCKETSPHINX_INCLUDE_DIRS} ${SPHINXBASE_INCLUDE_DIRS})
      add_definitions(-DHAS_POCKETSPHINX)
      message(STATUS "PocketSphinx found - voice input enabled for Linux")
    else()
      message(WARNING "PocketSphinx not found - voice input will be disabled on Linux")
    endif()
  endif()
else()
  # Fallback stub implementation
  list(APPEND SOURCES platform/StubVoiceInput.cpp)
  list(APPEND HEADERS platform/StubVoiceInput.h)
  message(STATUS "Voice input not supported on this platform - using stub implementation")
endif()

# Always include stub as fallback
list(APPEND SOURCES platform/StubVoiceInput.cpp)
list(APPEND HEADERS platform/StubVoiceInput.h)

core_add_library(semantic_voice)
