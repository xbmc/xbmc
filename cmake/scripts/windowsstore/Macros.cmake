function(core_link_library lib wraplib)
  message(AUTHOR_WARNING "core_link_library is not compatible with windows.")
endfunction()

function(find_soname lib)
  # Windows uses hardcoded dlls in xbmc/DllPaths_win32.h.
  # Therefore the output of this function is unused.
endfunction()

# Add precompiled header to target
# Arguments:
#   target existing target that will be set up to compile with a precompiled header
#   pch_header the precompiled header file
#   pch_source the precompiled header source file
# Optional Arguments:
#   PCH_TARGET build precompiled header as separate target with the given name
#              so that the same precompiled header can be used for multiple libraries
#   EXCLUDE_SOURCES if not all target sources shall use the precompiled header,
#                   the relevant files can be listed here
# On return:
#   Compiles the pch_source into a precompiled header and adds the header to
#   the given target
function(add_precompiled_header target pch_header pch_source)
  cmake_parse_arguments(PCH "" "PCH_TARGET" "EXCLUDE_SOURCES" ${ARGN})

  if(PCH_PCH_TARGET)
    set(pch_binary ${PRECOMPILEDHEADER_DIR}/${PCH_PCH_TARGET}.pch)
  else()
    set(pch_binary ${PRECOMPILEDHEADER_DIR}/${target}.pch)
  endif()

  # Set compile options and dependency for sources
  get_target_property(sources ${target} SOURCES)
  list(REMOVE_ITEM sources ${pch_source})
  foreach(exclude_source IN LISTS PCH_EXCLUDE_SOURCES)
    list(REMOVE_ITEM sources ${exclude_source})
  endforeach()
  set_source_files_properties(${sources}
                              PROPERTIES COMPILE_FLAGS "/Yu\"${pch_header}\" /Fp\"${pch_binary}\" /FI\"${pch_header}\""
                              OBJECT_DEPENDS "${pch_binary}")

  # Set compile options for precompiled header
  if(NOT PCH_PCH_TARGET OR NOT TARGET ${PCH_PCH_TARGET}_pch)
    set_source_files_properties(${pch_source}
                                PROPERTIES COMPILE_FLAGS "/Yc\"${pch_header}\" /Fp\"${pch_binary}\""
                                OBJECT_OUTPUTS "${pch_binary}")
  endif()

  # Compile precompiled header
  if(PCH_PCH_TARGET)
    # As own target for usage in multiple libraries
    if(NOT TARGET ${PCH_PCH_TARGET}_pch)
      add_library(${PCH_PCH_TARGET}_pch STATIC ${pch_source})
      set_target_properties(${PCH_PCH_TARGET}_pch PROPERTIES COMPILE_PDB_NAME vc140
                                                             COMPILE_PDB_OUTPUT_DIRECTORY ${PRECOMPILEDHEADER_DIR}
                                                             FOLDER "Build Utilities")
    endif()
    # From VS2012 onwards, precompiled headers have to be linked against (LNK2011).
    target_link_libraries(${target} PUBLIC ${PCH_PCH_TARGET}_pch)
    set_target_properties(${target} PROPERTIES COMPILE_PDB_NAME vc140
                                               COMPILE_PDB_OUTPUT_DIRECTORY ${PRECOMPILEDHEADER_DIR})
  else()
    # As part of the target
    target_sources(${target} PRIVATE ${pch_source})
  endif()
endfunction()

macro(winstore_set_assets target)
  file(GLOB ASSET_FILES "${CMAKE_SOURCE_DIR}/tools/windows/packaging/uwp/media/*.png")
  set_property(SOURCE ${ASSET_FILES} PROPERTY VS_DEPLOYMENT_CONTENT 1)
  set_property(SOURCE ${ASSET_FILES} PROPERTY VS_DEPLOYMENT_LOCATION "media")
  source_group("media" FILES ${ASSET_FILES})
  set(RESOURCES ${RESOURCES} ${ASSET_FILES}
                            "${CMAKE_SOURCE_DIR}/tools/windows/packaging/uwp/kodi_temp_key.pfx")

  set(LICENSE_FILES
    ${CMAKE_SOURCE_DIR}/LICENSE.md
    ${CMAKE_SOURCE_DIR}/privacy-policy.txt)
  if(EXISTS "${CMAKE_SOURCE_DIR}/known_issues.txt")
    list(APPEND LICENSE_FILES ${CMAKE_SOURCE_DIR}/known_issues.txt)
  endif()
  set_property(SOURCE ${LICENSE_FILES} PROPERTY VS_DEPLOYMENT_CONTENT 1)
  list(APPEND RESOURCES ${LICENSE_FILES})
endmacro()

macro(winstore_generate_manifest target)
  configure_file(
    ${CMAKE_SOURCE_DIR}/tools/windows/packaging/uwp/${APP_MANIFEST_NAME}.in
    ${CMAKE_CURRENT_BINARY_DIR}/${APP_MANIFEST_NAME}
    @ONLY)
  set(RESOURCES ${RESOURCES} ${CMAKE_CURRENT_BINARY_DIR}/${APP_MANIFEST_NAME})
endmacro()

macro(add_deployment_content_group path link match exclude)
  set(_link "")
  set(_exclude "")
  file(TO_NATIVE_PATH ${path} _path)
  file(TO_NATIVE_PATH ${match} _match)
  if (NOT "${link}" STREQUAL "")
    file(TO_NATIVE_PATH ${link} _link)
    set(_link "${_link}\\")
  endif()
  if(NOT "${exclude}" STREQUAL "")
    string(REPLACE "/" "\\" _exclude ${exclude})
  endif()
  string(CONCAT UWP_DEPLOYMENT_CONTENT_STR "${UWP_DEPLOYMENT_CONTENT_STR}"
    "    <EmbedResources Include=\"${_path}\\${_match}\" Exclude=\"${_exclude}\">\n"
    "      <Link>${_link}%(RecursiveDir)%(FileName)%(Extension)</Link>\n"
    "      <DeploymentContent>true</DeploymentContent>\n"
    "    </EmbedResources>\n")
endmacro()

macro(winstore_append_props target)
  # exclude debug dlls from packaging
  set(DEBUG_DLLS zlibd.dll)
  foreach(_dll ${DEBUG_DLLS})
    if (DEBUG_DLLS_EXCLUDE)
      list(APPEND DEBUG_DLLS_EXCLUDE "\;$(BuildRootPath)/${_dll}")
    else()
      list(APPEND DEBUG_DLLS_EXCLUDE "$(BuildRootPath)/${_dll}")
    endif()
    string(CONCAT DEBUG_DLLS_LINKAGE_PROPS "${DEBUG_DLLS_LINKAGE_PROPS}"
    "  <ItemGroup Label=\"Binaries\">\n"
    "    <None Include=\"$(BinPath)\\${_dll}\" Condition=\"'$(Configuration)'=='Debug'\">\n"
    "      <DeploymentContent>true</DeploymentContent>\n"
    "    </None>\n"
    "  </ItemGroup>\n")
  endforeach(_dll DEBUG_DLLS)

  add_deployment_content_group($(BuildRootPath) "" *.dll "${DEBUG_DLLS_EXCLUDE}")
  add_deployment_content_group($(BuildRootPath)/system system **/* "$(BuildRootPath)/**/shaders/**")
  add_deployment_content_group($(BuildRootPath)/system/shaders system/shaders **/*.fx "")
  add_deployment_content_group($(BuildRootPath)/media media **/* "")
  add_deployment_content_group($(BuildRootPath)/userdata userdata **/* "")
  add_deployment_content_group($(BuildRootPath)/addons addons **/* "")
  add_deployment_content_group($(BinaryAddonsPath) addons **/* "")

  foreach(xbt_file ${XBT_FILES})
    file(RELATIVE_PATH relative ${CMAKE_CURRENT_BINARY_DIR} ${xbt_file})
    file(TO_NATIVE_PATH ${relative} relative)
    string(CONCAT XBT_FILE_PROPS "${XBT_FILE_PROPS}"
    "  <ItemGroup Label=\"SkinsMedia\">\n"
    "    <None Include=\"$(BuildRootPath)\\${relative}\">\n"
    "      <Link>${relative}</Link>\n"
    "      <DeploymentContent>true</DeploymentContent>\n"
    "    </None>\n"
    "  </ItemGroup>\n")
  endforeach()

  set(VCPROJECT_PROPS_FILE "${CMAKE_CURRENT_BINARY_DIR}/${target}.props")
  file(TO_NATIVE_PATH ${DEPENDS_PATH} DEPENDENCIES_DIR_NATIVE)
  file(TO_NATIVE_PATH ${CMAKE_CURRENT_BINARY_DIR} CMAKE_CURRENT_BINARY_DIR_NATIVE)
  file(TO_NATIVE_PATH ${CMAKE_SOURCE_DIR}/project/Win32BuildSetup/BUILD_WIN32/addons BINARY_ADDONS_DIR_NATIVE)

  file(WRITE ${VCPROJECT_PROPS_FILE}
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
    "<Project ToolsVersion=\"4.0\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\n"
    "  <ImportGroup Label=\"PropertySheets\" />\n"
    "  <PropertyGroup Label=\"APP_DLLS\">\n"
    "    <BinPath>${DEPENDENCIES_DIR_NATIVE}\\bin</BinPath>\n"
    "    <BuildRootPath>${CMAKE_CURRENT_BINARY_DIR_NATIVE}\\$(Configuration)</BuildRootPath>\n"
    "    <BinaryAddonsPath>${BINARY_ADDONS_DIR_NATIVE}</BinaryAddonsPath>\n"
    "  </PropertyGroup>\n"
    "${DEBUG_DLLS_LINKAGE_PROPS}"
    "${XBT_FILE_PROPS}"
    "  <ItemGroup>\n"
    "${UWP_DEPLOYMENT_CONTENT_STR}"
    "  </ItemGroup>\n"
    "  <Target Name=\"_CollectCustomResources\" Inputs=\"@(EmbedResources)\" Outputs=\"@(EmbedResources->'$(OutputPath)\\PackageLayout\\%(Link)')\" BeforeTargets=\"AssignTargetPaths\">\n"
    "    <Message Text=\"Collecting package resources...\"/>\n"
    "    <ItemGroup>\n"
    "      <None Include=\"@(EmbedResources)\" />\n"
    "    </ItemGroup>\n"
    "  </Target>\n"
    "</Project>")
endmacro()

macro(winstore_add_target_properties target)
  winstore_set_assets(${target})
  winstore_generate_manifest(${target})
  winstore_append_props(${target})
endmacro()
