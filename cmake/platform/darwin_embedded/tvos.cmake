list(APPEND ARCH_DEFINES -DTARGET_DARWIN_TVOS)
set(ENABLE_AIRTUNES OFF CACHE BOOL "" FORCE)
set(${CORE_PLATFORM_NAME_LC}_SEARCH_CONFIG NO_DEFAULT_PATH CACHE STRING "")
set(PLATFORM_OPTIONAL_DEPS_EXCLUDE CEC)

