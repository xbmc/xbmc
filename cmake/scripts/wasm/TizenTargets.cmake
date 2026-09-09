# Stage a Tizen web app wrapper around wasm build artifacts.
set(WASM_TIZEN_TEMPLATE_DIR "${CMAKE_SOURCE_DIR}/tools/wasm/tizen")
set(WASM_TIZEN_STAGE_DIR "${CMAKE_BINARY_DIR}/tizen_packaging/Kodi")
set(WASM_TIZEN_OUTPUT_NAME "Kodi")
set(WASM_TIZEN_BUILD_TYPE "Debug")
set(WASM_TIZEN_PROFILE "tv-samsung")
set(WASM_TIZEN_API_VERSION "10.0")
set(WASM_TIZEN_PROJECT_YAML "${WASM_TIZEN_STAGE_DIR}/tizen_web_project.yaml")
set(WASM_TIZEN_WGT_PATH "${WASM_TIZEN_STAGE_DIR}/${WASM_TIZEN_BUILD_TYPE}/${WASM_TIZEN_OUTPUT_NAME}.wgt")

configure_file(
  "${WASM_TIZEN_TEMPLATE_DIR}/tizen_web_project.yaml.in"
  "${WASM_TIZEN_PROJECT_YAML}"
  @ONLY)

add_custom_target(package_tizen
  COMMAND ${CMAKE_COMMAND} -E make_directory "${WASM_TIZEN_STAGE_DIR}/media"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${WASM_TIZEN_TEMPLATE_DIR}/.project"
          "${WASM_TIZEN_TEMPLATE_DIR}/.tproject"
          "${WASM_TIZEN_TEMPLATE_DIR}/config.xml"
          "${WASM_TIZEN_TEMPLATE_DIR}/icon.png"
          "${WASM_TIZEN_STAGE_DIR}"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${WASM_TIZEN_PROJECT_YAML}"
          "${WASM_TIZEN_STAGE_DIR}/tizen_web_project.yaml"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${CMAKE_SOURCE_DIR}/tools/wasm/kodi.html"
          "${WASM_TIZEN_STAGE_DIR}/index.html"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${CMAKE_SOURCE_DIR}/media/splash.jpg"
          "${WASM_TIZEN_STAGE_DIR}/media/splash.jpg"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${CMAKE_BINARY_DIR}/${APP_NAME_LC}.js"
          "${CMAKE_BINARY_DIR}/${APP_NAME_LC}.wasm"
          "${CMAKE_BINARY_DIR}/${APP_NAME_LC}.data"
          "${WASM_TIZEN_STAGE_DIR}"
  DEPENDS ${APP_NAME_LC}
  COMMENT "Staging Tizen web app in ${WASM_TIZEN_STAGE_DIR}"
  VERBATIM
)

set_target_properties(package_tizen PROPERTIES FOLDER "Build Utilities")

# Resolve Tizen CLI (tz) from environment-driven SDK paths first.
find_program(WASM_TIZEN_TZ_EXECUTABLE
  NAMES tz
  HINTS
    "$ENV{TIZEN_CLI_PATH}"
    "$ENV{TIZEN_TOOLS_PATH}/tizen-core"
    "$ENV{TIZEN_SDK}/tools/tizen-core"
    "$ENV{TIZEN_SDK_ROOT}/tools/tizen-core"
  NO_DEFAULT_PATH
)

# Fallback to PATH lookup if env-based lookup did not resolve tz.
if(NOT WASM_TIZEN_TZ_EXECUTABLE)
  find_program(WASM_TIZEN_TZ_EXECUTABLE NAMES tz)
endif()

if(WASM_TIZEN_TZ_EXECUTABLE)
  add_custom_target(package_tizen_wgt
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${WASM_TIZEN_STAGE_DIR}/${WASM_TIZEN_BUILD_TYPE}"
    COMMAND "${WASM_TIZEN_TZ_EXECUTABLE}" pack -w "${WASM_TIZEN_STAGE_DIR}" -t wgt
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${WASM_TIZEN_STAGE_DIR}/${WASM_TIZEN_BUILD_TYPE}/projects"
    DEPENDS package_tizen
    COMMENT "Packaging Tizen WGT via ${WASM_TIZEN_TZ_EXECUTABLE}"
    VERBATIM
  )
  set_target_properties(package_tizen_wgt PROPERTIES FOLDER "Build Utilities")

  # tz install targets the only connected device unless one is named. Cache
  # variables win over the environment so a configure-time choice is sticky.
  if(NOT WASM_TIZEN_INSTALL_TARGET AND DEFINED ENV{TIZEN_TARGET_NAME})
    set(WASM_TIZEN_INSTALL_TARGET "$ENV{TIZEN_TARGET_NAME}")
  endif()
  if(NOT WASM_TIZEN_INSTALL_SERIAL AND DEFINED ENV{TIZEN_TARGET_SERIAL})
    set(WASM_TIZEN_INSTALL_SERIAL "$ENV{TIZEN_TARGET_SERIAL}")
  endif()

  set(WASM_TIZEN_INSTALL_DEVICE_ARGS "")
  if(WASM_TIZEN_INSTALL_SERIAL)
    list(APPEND WASM_TIZEN_INSTALL_DEVICE_ARGS --serial "${WASM_TIZEN_INSTALL_SERIAL}")
  elseif(WASM_TIZEN_INSTALL_TARGET)
    list(APPEND WASM_TIZEN_INSTALL_DEVICE_ARGS --target "${WASM_TIZEN_INSTALL_TARGET}")
  endif()

  add_custom_target(install_tizen_wgt
    COMMAND "${WASM_TIZEN_TZ_EXECUTABLE}" install
            --package-path "${WASM_TIZEN_WGT_PATH}"
            ${WASM_TIZEN_INSTALL_DEVICE_ARGS}
    DEPENDS package_tizen_wgt
    COMMENT "Installing Tizen WGT"
    VERBATIM
  )
  set_target_properties(install_tizen_wgt PROPERTIES FOLDER "Build Utilities")

  get_filename_component(WASM_TIZEN_CORE_DIR "${WASM_TIZEN_TZ_EXECUTABLE}" DIRECTORY)
  find_program(WASM_TIZEN_SDB_EXECUTABLE
    NAMES sdb
    HINTS "${WASM_TIZEN_CORE_DIR}/.." "$ENV{TIZEN_SDK}/tools" "$ENV{TIZEN_SDK_ROOT}/tools"
  )
  if(WASM_TIZEN_SDB_EXECUTABLE)
    add_custom_target(inspect_tizen
      COMMAND ${CMAKE_COMMAND} -E env
              "SDB=${WASM_TIZEN_SDB_EXECUTABLE}"
              "TIZEN_TARGET_SERIAL=${WASM_TIZEN_INSTALL_SERIAL}"
              sh "${WASM_TIZEN_TEMPLATE_DIR}/inspect.sh"
      COMMENT "Launching Kodi on the TV under the Web Inspector"
      USES_TERMINAL
      VERBATIM
    )
    set_target_properties(inspect_tizen PROPERTIES FOLDER "Build Utilities")
  endif()
else()
  message(STATUS "WASM: tz CLI not found. Set TIZEN_CLI_PATH/TIZEN_TOOLS_PATH/TIZEN_SDK/TIZEN_SDK_ROOT to enable package_tizen_wgt.")
endif()
