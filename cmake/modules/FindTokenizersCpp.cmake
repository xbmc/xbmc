#.rst:
# FindTokenizersCpp
# -----------------
# Finds the HuggingFace tokenizers-cpp library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::TokenizersCpp   - The tokenizers-cpp library
#
# OPTIONAL MODULE: tokenizers-cpp is NOT required for Kodi's semantic search.
# Kodi includes a built-in WordPiece tokenizer implementation (Tokenizer.h/.cpp)
# that is compatible with BERT and sentence-transformers models.
#
# This module is provided as an optional alternative for users who:
# - Already have tokenizers-cpp installed on their system
# - Want to use the full HuggingFace tokenizers library features
# - Need advanced tokenization capabilities beyond WordPiece
#
# To use tokenizers-cpp instead of the built-in tokenizer, set:
#   -DUSE_TOKENIZERS_CPP=ON
#
# Otherwise, the built-in tokenizer will be used (recommended).

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  # Only search if explicitly enabled
  if(USE_TOKENIZERS_CPP)
    include(cmake/scripts/common/ModuleHelpers.cmake)

    set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC tokenizers_cpp)

    SETUP_BUILD_VARS()

    # Look for tokenizers-cpp using pkg-config
    SETUP_FIND_SPECS()
    SEARCH_EXISTING_PACKAGES()

    # If pkg-config didn't find it, try manual search
    if(NOT ${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
      # Try to find tokenizers-cpp headers
      find_path(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR
          NAMES tokenizers_cpp.h
          PATH_SUFFIXES tokenizers_cpp
          HINTS ${DEPENDS_PATH}/include
      )

      find_library(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY
          NAMES tokenizers_cpp
          HINTS ${DEPENDS_PATH}/lib
      )

      if(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR AND ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY)
        set(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND TRUE)
      endif()
    endif()

    if(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
      # If found via pkg-config, create alias
      if(TARGET PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
        add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
      else()
        # Create imported target for manual find
        add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
        set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
            IMPORTED_LOCATION "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR}"
        )
      endif()

      # Set compile definition to use external tokenizer
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_COMPILE_DEFINITIONS HAS_TOKENIZERS_CPP)
      ADD_TARGET_COMPILE_DEFINITION()

      message(STATUS "Found tokenizers-cpp - using external tokenizer library")
    else()
      if(TokenizersCpp_FIND_REQUIRED)
        message(FATAL_ERROR "tokenizers-cpp library was not found. "
                            "Either install tokenizers-cpp or disable USE_TOKENIZERS_CPP.")
      else()
        message(STATUS "tokenizers-cpp not found - using built-in WordPiece tokenizer (recommended)")
      endif()
    endif()
  else()
    # tokenizers-cpp is not enabled, use built-in tokenizer
    message(STATUS "Using built-in WordPiece tokenizer (tokenizers-cpp disabled)")
  endif()
endif()
