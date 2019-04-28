set(PLATFORM_BUNDLE_IDENTIFIER "${APP_PACKAGE}-${CORE_PLATFORM_NAME_LC}")
list(APPEND final_message "Bundle ID: ${PLATFORM_BUNDLE_IDENTIFIER}")
include(cmake/scripts/osx/PathSetup.cmake)
