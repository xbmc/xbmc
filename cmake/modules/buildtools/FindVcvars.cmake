# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
FindVcvars
----------

Finds a "vcvars" batch script.

The module can be used when configuring a project or when running
in cmake -P script mode.

These variables can be used to choose which "vcvars" batch script is looked up.

.. variable:: Vcvars_MSVC_ARCH

  Possible values are `32` or `64`

  If not explicitly set in the calling scope, the variable is initialized
  based on the value of :variable:`CMAKE_SIZEOF_VOID_P` in configuration mode, and
  to 64 in script mode.

.. variable:: Vcvars_MSVC_VERSION

  Possible values corresponds to :variable:`MSVC_VERSION`.

  If not explicitly set in the calling scope, :variable:`Vcvars_MSVC_VERSION` is
  initialized using :variable:`MSVC_VERSION` variable if it is defined, otherwise
  the variable :variable:`Vcvars_MSVC_VERSION` is initialized based on the most
  recent version of Visual Studio installed on the system.

.. variable:: Vcvars_FIND_VCVARSALL

  If set to TRUE, forces the module to look up ``vcvarsall.bat`` instead of
  an architecture-specific script such as ``vcvars64.bat``.

  When enabled, the appropriate architecture argument (e.g., ``x64`` or ``x86``),
  determined from the value of :variable:`Vcvars_MSVC_ARCH`, is automatically passed
  to ``vcvarsall.bat`` via the generated wrapper script.

  Default is FALSE.

.. variable:: Vcvars_PLATFORM_TOOLSET_VERSION

  The version of the Visual C++ toolset passed to ``vcvarsall.bat``
  via the ``-vcvars_ver=`` argument.

  If not explicitly set, this variable is initialized as follows:

  * If :variable:`CMAKE_VS_PLATFORM_TOOLSET_VERSION` is defined, its full value
    (e.g., ``14.28.29910``) is used directly.
  * Otherwise, a simplified version (e.g., ``14.3``) is derived from
    :variable:`Vcvars_MSVC_VERSION` using a known mapping provided by
    :command:`Vcvars_ConvertMsvcVersionToVcToolsetVersion`.

  Only used when :variable:`Vcvars_FIND_VCVARSALL` is ``TRUE``.


This will define the following variables:

.. variable:: Vcvars_BATCH_FILE

  Path to ``vcvars32.bat``, ``vcvarsamd64.bat`` or ``vcvars64.bat``.

.. variable:: Vcvars_LAUNCHER

  Path to a generated wrapper script allowing to execute program after
  setting environment defined by `Vcvars_BATCH_FILE`.

  It can be used within :module:`ExternalProject` steps
  specifying command like this::

    set(cmd_wrapper)
    if(MSVC)
      find_package(Vcvars REQUIRED)
      set(cmd_wrapper ${Vcvars_LAUNCHER})
    endif()

    ExternalProject_Add(AwesomeProject
      [...]
      BUILD_COMMAND ${cmd_wrapper} <command> arg1 arg2 [...]
      [...]
      )

This module also defines the following functions


.. command:: Vcvars_GetVisualStudioPaths

  The ``Vcvars_GetVisualStudioPaths()`` function returns a list of all
  possible Visual Studio registry paths associated with a given ``<msvc_version>``
  and ``<msvc_arch>``::

    Vcvars_GetVisualStudioPaths(<msvc_version> <msvc_arch> <output_var>)

  The options are:

  ``<msvc_version>``
    Specify the Visual Studio compiler version. See :variable:`MSVC_VERSION`
    for possible values.

  ``<msvc_arch>``
    Specify the Visual Studio architecture. Possible values are `32` or `64`.

  ``<output_var>``
    The name of the variable to be set with the list of registry paths.


.. command:: Vcvars_ConvertMsvcVersionToVsVersion

  The ``Vcvars_ConvertMsvcVersionToVsVersion()`` function converts a
  :variable:`MSVC_VERSION` of the form ``NNNN`` to a Visual Studio version
  of the form ``XX.Y`::

    Vcvars_ConvertMsvcVersionToVsVersion(<msvc_version> <output_var>)

  The options are:

  ``<msvc_version>``
    Specify the Visual Studio compiler version. See :variable:`MSVC_VERSION`
    for possible values.

  ``<output_var>``
    The name of the variable to be set with the Visual Studio version.


.. command:: Vcvars_ConvertMsvcVersionToVcToolsetVersion

  Converts an MSVC version number (e.g., ``1931``) to its corresponding
  Visual C++ toolset version (e.g., ``14.3``).

  The result is suitable for use in the ``-vcvars_ver=`` argument
  passed to ``vcvarsall.bat``.

  This function currently returns toolset versions with only a single
  digit after the decimal point. For more specific control (e.g.,
  ``14.28.29910``), use :variable:`CMAKE_VS_PLATFORM_TOOLSET_VERSION`
  or set :variable:`Vcvars_PLATFORM_TOOLSET_VERSION` explicitly.

  ::

    Vcvars_ConvertMsvcVersionToVcToolsetVersion(<msvc_version> <output_var>)


.. command:: Vcvars_FindFirstValidMsvcVersion

  The `Vcvars_FindFirstValidMsvcVersion()` function identifies the first MSVC version from a list of candidates that has an associated and discoverable `vcvars` batch script::

  ```
  Vcvars_FindFirstValidMsvcVersion(<msvc_arch> <candidate_msvc_versions> <msvc_version_output_varname> <batch_file_output_varname>)
  ```

  The options are:

  ``<msvc_arch>``
    Specify the Visual Studio architecture. Possible values are `32` or `64`.

  ``<candidate_msvc_versions>``
    A list of MSVC version numbers to check (e.g., `1949;1948;1947`). The list should be ordered from most to least preferred.

  ``<msvc_version_output_varname>``
    The name of the variable to store the first matching MSVC version.

  ``<batch_file_output_varname>``
    The name of the variable to store the path to the corresponding `vcvars` batch script.

  This is typically used internally to identify the most recent supported Visual Studio installation that provides the appropriate environment setup scripts, but it can also be reused in custom detection logic when running in configuration or script mode.


The module also defines the following variables mapping MSVC versions
to their associated toolset or Visual Studio major release:

.. variable:: Vcvars_TOOLSET_<Toolset>_MSVC_VERSIONS

  Lists the MSVC version numbers associated with a specific compiler toolset.
  These are grouped based on official Microsoft and CMake-maintained mappings.

  Example mappings:

  - ``Vcvars_TOOLSET_143_MSVC_VERSIONS`` — MSVC versions associated with toolset ``v143`` (Visual Studio 2022)
  - ``Vcvars_TOOLSET_142_MSVC_VERSIONS`` — MSVC versions associated with toolset ``v142`` (Visual Studio 2019)
  - ``Vcvars_TOOLSET_141_MSVC_VERSIONS`` — MSVC versions associated with toolset ``v141`` (Visual Studio 2017)
  - ``Vcvars_TOOLSET_140_MSVC_VERSIONS`` — MSVC versions associated with toolset ``v140`` (Visual Studio 2015)
  - ``Vcvars_TOOLSET_120_MSVC_VERSIONS`` — MSVC versions associated with toolset ``v120`` (Visual Studio 2013)
  - ``Vcvars_TOOLSET_110_MSVC_VERSIONS`` — MSVC versions associated with toolset ``v110`` (Visual Studio 2012)
  - ``Vcvars_TOOLSET_100_MSVC_VERSIONS`` — MSVC versions associated with toolset ``v100`` (Visual Studio 2010)
  - ``Vcvars_TOOLSET_90_MSVC_VERSIONS`` — MSVC versions associated with toolset ``v90``  (Visual Studio 2008)
  - ``Vcvars_TOOLSET_80_MSVC_VERSIONS`` — MSVC versions associated with toolset ``v80``  (Visual Studio 2005)

.. variable:: Vcvars_VS<Major>_MSVC_VERSIONS

  Lists the MSVC version numbers grouped by the corresponding Visual Studio release.

  These aliases mirror the toolset mappings above but allow for intuitive access
  based on the Visual Studio version number.

  Example aliases:

  - ``Vcvars_VS17_MSVC_VERSIONS`` — aliases ``Vcvars_TOOLSET_143_MSVC_VERSIONS`` (Visual Studio 2022)
  - ``Vcvars_VS16_MSVC_VERSIONS`` — aliases ``Vcvars_TOOLSET_142_MSVC_VERSIONS`` (Visual Studio 2019)
  - ``Vcvars_VS15_MSVC_VERSIONS`` — aliases ``Vcvars_TOOLSET_141_MSVC_VERSIONS`` (Visual Studio 2017)
  - ``Vcvars_VS14_MSVC_VERSIONS`` — aliases ``Vcvars_TOOLSET_140_MSVC_VERSIONS`` (Visual Studio 2015)
  - ``Vcvars_VS12_MSVC_VERSIONS`` — aliases ``Vcvars_TOOLSET_120_MSVC_VERSIONS`` (Visual Studio 2013)
  - ``Vcvars_VS11_MSVC_VERSIONS`` — aliases ``Vcvars_TOOLSET_110_MSVC_VERSIONS`` (Visual Studio 2012)
  - ``Vcvars_VS10_MSVC_VERSIONS`` — aliases ``Vcvars_TOOLSET_100_MSVC_VERSIONS`` (Visual Studio 2010)
  - ``Vcvars_VS9_MSVC_VERSIONS`` — aliases ``Vcvars_TOOLSET_90_MSVC_VERSIONS``  (Visual Studio 2008)
  - ``Vcvars_VS8_MSVC_VERSIONS`` — aliases ``Vcvars_TOOLSET_80_MSVC_VERSIONS``  (Visual Studio 2005)

These variables are primarily intended for internal use and test coverage but may also be
useful in advanced configuration or filtering of available toolchains.


This module also supports the following COMPONENTS:

* ``FunctionsOnly``: Only defines the helper functions
  (e.g., ``Vcvars_GetVisualStudioPaths``, ``Vcvars_ConvertMsvcVersionToVsVersion``)
  without attempting to discover or set ``Vcvars_BATCH_FILE`` or ``Vcvars_LAUNCHER``.

#]=======================================================================]

cmake_minimum_required(VERSION 3.20.6...3.22.6 FATAL_ERROR)

# See https://github.com/Kitware/CMake/blob/v4.2.1/Modules/Platform/Windows-MSVC.cmake#L64-L96

set(Vcvars_TOOLSET_145_MSVC_VERSIONS # VS 2026
  1950
  )
set(Vcvars_TOOLSET_143_MSVC_VERSIONS # VS 2022
  1949 1948 1947 1946 1945 1944 1943 1942 1941 1940
  1939 1938 1937 1936 1935 1934 1933 1932 1931 1930
  )
set(Vcvars_TOOLSET_142_MSVC_VERSIONS # VS 2019
  1929 1928 1927 1926 1925 1924 1923 1922 1921 1920
  )
set(Vcvars_TOOLSET_141_MSVC_VERSIONS # VS 2017
  1916 1915 1914 1913 1912 1911 1910
  )
set(Vcvars_TOOLSET_140_MSVC_VERSIONS 1900) # VS 2015
set(Vcvars_TOOLSET_120_MSVC_VERSIONS 1800) # VS 2013
set(Vcvars_TOOLSET_110_MSVC_VERSIONS 1700) # VS 2012
set(Vcvars_TOOLSET_100_MSVC_VERSIONS 1600) # VS 2010
set(Vcvars_TOOLSET_90_MSVC_VERSIONS 1500) # VS 2008
set(Vcvars_TOOLSET_80_MSVC_VERSIONS 1400) # VS 2005
set(Vcvars_TOOLSET_71_MSVC_VERSIONS 1310) # VS 2003
set(Vcvars_TOOLSET_70_MSVC_VERSIONS 1300) # VS 2002
set(Vcvars_TOOLSET_60_MSVC_VERSIONS 1200) # VS 6.0

# See https://en.wikipedia.org/wiki/Microsoft_Visual_C%2B%2B#Internal_version_numbering
# and https://gitlab.kitware.com/cmake/cmake/-/merge_requests/9271
set(Vcvars_VS18_MSVC_VERSIONS ${Vcvars_TOOLSET_145_MSVC_VERSIONS}) # VS 2026
set(Vcvars_VS17_MSVC_VERSIONS ${Vcvars_TOOLSET_143_MSVC_VERSIONS}) # VS 2022
set(Vcvars_VS16_MSVC_VERSIONS ${Vcvars_TOOLSET_142_MSVC_VERSIONS}) # VS 2019
set(Vcvars_VS15_MSVC_VERSIONS ${Vcvars_TOOLSET_141_MSVC_VERSIONS}) # VS 2017
set(Vcvars_VS14_MSVC_VERSIONS ${Vcvars_TOOLSET_140_MSVC_VERSIONS}) # VS 2015
set(Vcvars_VS12_MSVC_VERSIONS ${Vcvars_TOOLSET_120_MSVC_VERSIONS}) # VS 2013
set(Vcvars_VS11_MSVC_VERSIONS ${Vcvars_TOOLSET_110_MSVC_VERSIONS}) # VS 2012
set(Vcvars_VS10_MSVC_VERSIONS ${Vcvars_TOOLSET_100_MSVC_VERSIONS}) # VS 2010
set(Vcvars_VS9_MSVC_VERSIONS ${Vcvars_TOOLSET_90_MSVC_VERSIONS}) # VS 2008
set(Vcvars_VS8_MSVC_VERSIONS ${Vcvars_TOOLSET_80_MSVC_VERSIONS}) # VS 2005
set(Vcvars_VS71_MSVC_VERSIONS ${Vcvars_TOOLSET_71_MSVC_VERSIONS}) # VS 2003
set(Vcvars_VS7_MSVC_VERSIONS ${Vcvars_TOOLSET_70_MSVC_VERSIONS}) # VS 2002
set(Vcvars_VS6_MSVC_VERSIONS ${Vcvars_TOOLSET_60_MSVC_VERSIONS}) # VS 6.0

# Global variables used only in this script (unset at the end)
set(_Vcvars_MSVC_ARCH_REGEX "^(32|64)$")
set(_Vcvars_MSVC_VERSION_REGEX "^[0-9][0-9][0-9][0-9]$")
set(_Vcvars_SUPPORTED_MSVC_VERSIONS
  ${Vcvars_TOOLSET_145_MSVC_VERSIONS} # VS 2026
  ${Vcvars_TOOLSET_143_MSVC_VERSIONS} # VS 2022
  ${Vcvars_TOOLSET_142_MSVC_VERSIONS} # VS 2019
  ${Vcvars_TOOLSET_141_MSVC_VERSIONS} # VS 2017
  ${Vcvars_TOOLSET_140_MSVC_VERSIONS} # VS 2015
  ${Vcvars_TOOLSET_120_MSVC_VERSIONS} # VS 2013
  ${Vcvars_TOOLSET_110_MSVC_VERSIONS} # VS 2012
  ${Vcvars_TOOLSET_100_MSVC_VERSIONS} # VS 2010
  ${Vcvars_TOOLSET_90_MSVC_VERSIONS} # VS 2008
  ${Vcvars_TOOLSET_80_MSVC_VERSIONS} # VS 2005
  ${Vcvars_TOOLSET_71_MSVC_VERSIONS} # VS 2003
  ${Vcvars_TOOLSET_70_MSVC_VERSIONS} # VS 2002
  ${Vcvars_TOOLSET_60_MSVC_VERSIONS} # VS 6.0
  )

# process component arguments
if(Vcvars_FIND_COMPONENTS)
  if("FunctionsOnly" IN_LIST Vcvars_FIND_COMPONENTS)
    set(_Vcvars_FUNCTIONS_ONLY TRUE)
  endif()
  if(_Vcvars_FUNCTIONS_ONLY AND NOT Vcvars_FIND_COMPONENTS STREQUAL "FunctionsOnly")
    message(FATAL_ERROR "FindVcvars: Only supported COMPONENTS argument is 'FunctionsOnly'.")
  endif()
endif()

function(_vcvars_message)
  if(NOT Vcvars_FIND_QUIETLY)
    message(${ARGN})
  endif()
endfunction()

function(Vcvars_ConvertMsvcVersionToVsVersion msvc_version output_var)
  if(NOT msvc_version MATCHES ${_Vcvars_MSVC_VERSION_REGEX})
    message(FATAL_ERROR "msvc_version is expected to match `${_Vcvars_MSVC_VERSION_REGEX}`")
  endif()
  # See https://en.wikipedia.org/wiki/Microsoft_Visual_C%2B%2B#Internal_version_numbering
  if(msvc_version IN_LIST Vcvars_VS18_MSVC_VERSIONS) # VS 2026
    set(vs_version "18")
  elseif(msvc_version IN_LIST Vcvars_VS17_MSVC_VERSIONS) # VS 2022
    set(vs_version "17")
  elseif(msvc_version IN_LIST Vcvars_VS16_MSVC_VERSIONS) # VS 2019
    set(vs_version "16")
  elseif(msvc_version IN_LIST Vcvars_VS15_MSVC_VERSIONS) # VS 2017
    set(vs_version "15")
  elseif(msvc_version IN_LIST Vcvars_VS14_MSVC_VERSIONS) # VS 2015
    set(vs_version "14.0")
  elseif(msvc_version IN_LIST Vcvars_VS12_MSVC_VERSIONS) # VS 2013
    set(vs_version "12.0")
  elseif(msvc_version IN_LIST Vcvars_VS11_MSVC_VERSIONS) # VS 2012
    set(vs_version "11.0")
  elseif(msvc_version IN_LIST Vcvars_VS10_MSVC_VERSIONS) # VS 2010
    set(vs_version "10.0")
  elseif(msvc_version IN_LIST Vcvars_VS9_MSVC_VERSIONS) # VS 2008
    set(vs_version "9.0")
  elseif(msvc_version IN_LIST Vcvars_VS8_MSVC_VERSIONS) # VS 2005
    set(vs_version "8.0")
  elseif(msvc_version IN_LIST Vcvars_VS71_MSVC_VERSIONS) # VS 2003
    set(vs_version "7.1")
  elseif(msvc_version IN_LIST Vcvars_VS7_MSVC_VERSIONS) # VS 2002
    set(vs_version "7.0")
  elseif(msvc_version IN_LIST Vcvars_VS6_MSVC_VERSIONS) # VS 6.0
    set(vs_version "6.0")
  else()
    message(FATAL_ERROR "failed to convert msvc_version [${msvc_version}]. It is not a known version number.")
  endif()
  set(${output_var} ${vs_version} PARENT_SCOPE)
endfunction()

function(Vcvars_ConvertMsvcVersionToVcToolsetVersion msvc_version output_var)
  if(NOT msvc_version MATCHES ${_Vcvars_MSVC_VERSION_REGEX})
    message(FATAL_ERROR "msvc_version is expected to match `${_Vcvars_MSVC_VERSION_REGEX}`")
  endif()
  if(msvc_version IN_LIST Vcvars_TOOLSET_145_MSVC_VERSIONS) # VS 2026
    set(vc_toolset_version "14.5")
  elseif(msvc_version IN_LIST Vcvars_TOOLSET_143_MSVC_VERSIONS) # VS 2022
    set(vc_toolset_version "14.3")
  elseif(msvc_version IN_LIST Vcvars_TOOLSET_142_MSVC_VERSIONS) # VS 2019
    set(vc_toolset_version "14.2")
  elseif(msvc_version IN_LIST Vcvars_TOOLSET_141_MSVC_VERSIONS) # VS 2017
    set(vc_toolset_version "14.1")
  elseif(msvc_version IN_LIST Vcvars_TOOLSET_140_MSVC_VERSIONS) # VS 2015
    set(vc_toolset_version "14.0")
  elseif(msvc_version IN_LIST Vcvars_TOOLSET_120_MSVC_VERSIONS) # VS 2013
    set(vc_toolset_version "12.0")
  elseif(msvc_version IN_LIST Vcvars_TOOLSET_110_MSVC_VERSIONS) # VS 2012
    set(vc_toolset_version "11.0")
  elseif(msvc_version IN_LIST Vcvars_TOOLSET_100_MSVC_VERSIONS) # VS 2010
    set(vc_toolset_version "10.0")
  elseif(msvc_version IN_LIST Vcvars_TOOLSET_90_MSVC_VERSIONS) # VS 2008
    set(vc_toolset_version "9.0")
  elseif(msvc_version IN_LIST Vcvars_TOOLSET_80_MSVC_VERSIONS) # VS 2005
    set(vc_toolset_version "8.0")
  elseif(msvc_version IN_LIST Vcvars_TOOLSET_71_MSVC_VERSIONS) # VS 2003
    set(vc_toolset_version "7.1")
  elseif(msvc_version IN_LIST Vcvars_TOOLSET_70_MSVC_VERSIONS) # VS 2002
    set(vc_toolset_version "7.0")
  elseif(msvc_version IN_LIST Vcvars_TOOLSET_60_MSVC_VERSIONS) # VS 6.0
    set(vc_toolset_version "6.0")
  else()
    message(FATAL_ERROR "failed to convert msvc_version [${msvc_version}] to VC toolset version. It is not a known version number.")
  endif()
  set(${output_var} ${vc_toolset_version} PARENT_SCOPE)
endfunction()

function(Vcvars_GetVisualStudioPaths msvc_version msvc_arch output_var)

  if(NOT msvc_version MATCHES ${_Vcvars_MSVC_VERSION_REGEX})
    message(FATAL_ERROR "msvc_version is expected to match `${_Vcvars_MSVC_VERSION_REGEX}`")
  endif()

  if(NOT msvc_arch MATCHES ${_Vcvars_MSVC_ARCH_REGEX})
    message(FATAL_ERROR "msvc_arch argument is expected to match '${_Vcvars_MSVC_ARCH_REGEX}'")
  endif()

  Vcvars_ConvertMsvcVersionToVsVersion(${msvc_version} vs_version)

  set(_vs_installer_paths "")
  set(_vs_registry_paths "")
  if(vs_version VERSION_GREATER_EQUAL "15.0")
    # Query the VS Installer tool for locations of VS 2017 and above.
    string(REGEX REPLACE "^([0-9]+)\.[0-9]+$" "\\1" vs_installer_version ${vs_version})
    cmake_host_system_information(RESULT _vs_dir QUERY VS_${vs_installer_version}_DIR)
    if(_vs_dir)
      list(APPEND _vs_installer_paths "${_vs_dir}/VC/Auxiliary/Build")
    endif()
  else()
    # Registry keys for locations of VS 2015 and below
    set(_hkeys
      "HKEY_USERS"
      "HKEY_CURRENT_USER"
      "HKEY_LOCAL_MACHINE"
      "HKEY_CLASSES_ROOT"
      )
    set(_suffixes
      ""
      "_Config"
      )
    set(_arch_path "bin/amd64")
    if(msvc_arch STREQUAL "32")
      set(_arch_path "bin")
    endif()
    set(_vs_registry_paths)
    foreach(_hkey IN LISTS _hkeys)
      foreach(_suffix IN LISTS _suffixes)
        set(_vc "VC")
        if(_vs_version STREQUAL "6.0")
          set(_vc "Microsoft Visual C++")
        endif()
        list(APPEND _vs_registry_paths
          "[${_hkey}\\SOFTWARE\\Microsoft\\VisualStudio\\${vs_version}${_suffix}\\Setup\\${_vc};ProductDir]/${_arch_path}"
          )
      endforeach()
    endforeach()
  endif()
  set(_vs_installer_paths ${_vs_installer_paths} ${_vs_registry_paths})
  if(_vs_installer_paths STREQUAL "")
    set(_vs_installer_paths "${output_var}-${msvc_version}-${msvc_arch}-NOTFOUND")
  endif()
  set(${output_var} ${_vs_installer_paths} PARENT_SCOPE)
endfunction()

function(Vcvars_FindFirstValidMsvcVersion msvc_arch candidate_msvc_versions msvc_version_output_varname batch_file_output_varname)
  _vcvars_message(STATUS "Setting ${msvc_version_output_varname}")
  # which vcvars script ?
  if(Vcvars_FIND_VCVARSALL)
    set(_candidate_scripts vcvarsall.bat)
  else()
    if(msvc_arch STREQUAL "64")
      set(_candidate_scripts vcvarsamd64.bat vcvars64.bat)
    else()
      set(_candidate_scripts vcvars32.bat)
    endif()
  endif()
  # set defaults
  set(_batch_file "${batch_file_output_varname}-NOTFOUND")
  set(_msvc_version "")
  foreach(_candidate_msvc_version IN LISTS candidate_msvc_versions)
    Vcvars_GetVisualStudioPaths(${_candidate_msvc_version} "${msvc_arch}" _paths)
    Vcvars_ConvertMsvcVersionToVsVersion(${_candidate_msvc_version} _candidate_vs_version)
    set(_msg "  Visual Studio ${_candidate_vs_version} (${_candidate_msvc_version})")
    _vcvars_message(STATUS "${_msg}")
    find_program(_batch_file NAMES ${_candidate_scripts}
      PATHS ${_paths}
      NO_CACHE
      )
    if(_batch_file)
      _vcvars_message(STATUS "${_msg} - found")
      set(_msvc_version ${_candidate_msvc_version})
      _vcvars_message(STATUS "Setting ${msvc_version_output_varname} to '${_msvc_version}' as it was the newest Visual Studio installed providing vcvars scripts")
      break()
    else()
      _vcvars_message(STATUS "${_msg} - not found")
    endif()
  endforeach()
  # set outputs
  set(${msvc_version_output_varname} ${_msvc_version} PARENT_SCOPE)
  set(${batch_file_output_varname} ${_batch_file} PARENT_SCOPE)
endfunction()

if(_Vcvars_FUNCTIONS_ONLY)
  set(Vcvars_FOUND TRUE)
  return()
endif()

# default
if(NOT DEFINED Vcvars_MSVC_ARCH)
  if(NOT DEFINED CMAKE_SIZEOF_VOID_P)
    set(Vcvars_MSVC_ARCH "64")
    _vcvars_message(STATUS "Setting Vcvars_MSVC_ARCH to '${Vcvars_MSVC_ARCH}' as CMAKE_SIZEOF_VOID_P was none")
  else()
    if("${CMAKE_SIZEOF_VOID_P}" EQUAL 4)
      set(Vcvars_MSVC_ARCH "32")
    elseif("${CMAKE_SIZEOF_VOID_P}" EQUAL 8)
      set(Vcvars_MSVC_ARCH "64")
    else()
      message(FATAL_ERROR "CMAKE_SIZEOF_VOID_P [${CMAKE_SIZEOF_VOID_P}] is expected to be either 4 or 8")
    endif()
    # Display message only once in config mode
    if(NOT DEFINED Vcvars_BATCH_FILE)
      _vcvars_message(STATUS "Setting Vcvars_MSVC_ARCH to '${Vcvars_MSVC_ARCH}' as CMAKE_SIZEOF_VOID_P was `${CMAKE_SIZEOF_VOID_P}`")
    endif()
  endif()
endif()

if(NOT DEFINED Vcvars_MSVC_VERSION)
  if(DEFINED MSVC_VERSION)
    set(Vcvars_MSVC_VERSION ${MSVC_VERSION})
    # Display message only once in config mode
    if(NOT DEFINED Vcvars_BATCH_FILE)
      _vcvars_message(STATUS "Setting Vcvars_MSVC_VERSION to '${Vcvars_MSVC_VERSION}' as MSVC_VERSION was `${MSVC_VERSION}`")
    endif()
  endif()
endif()

if(NOT DEFINED Vcvars_FIND_VCVARSALL)
  set(Vcvars_FIND_VCVARSALL FALSE)
endif()

# check Vcvars_MSVC_ARCH is properly set
if(NOT Vcvars_MSVC_ARCH MATCHES ${_Vcvars_MSVC_ARCH_REGEX})
  message(FATAL_ERROR "Vcvars_MSVC_ARCH [${Vcvars_MSVC_ARCH}] is expected to match `${_Vcvars_MSVC_ARCH_REGEX}`")
endif()

# auto-discover Vcvars_MSVC_VERSION value (and set Vcvars_BATCH_FILE as a side-effect)
if(NOT DEFINED Vcvars_MSVC_VERSION)
  Vcvars_FindFirstValidMsvcVersion(
    "${Vcvars_MSVC_ARCH}"
    "${_Vcvars_SUPPORTED_MSVC_VERSIONS}"
    Vcvars_MSVC_VERSION
    _batch_file
    )
  if(_batch_file)
    Vcvars_ConvertMsvcVersionToVsVersion(${Vcvars_MSVC_VERSION} _vs_version)
    set(Vcvars_BATCH_FILE ${_batch_file}
      CACHE FILEPATH "Visual Studio ${_vs_version} vcvars script"
      )
    unset(_vs_version)
  endif()
  unset(_batch_file)
endif()

if(NOT DEFINED Vcvars_PLATFORM_TOOLSET_VERSION)
  if(DEFINED CMAKE_VS_PLATFORM_TOOLSET_VERSION)
    set(Vcvars_PLATFORM_TOOLSET_VERSION ${CMAKE_VS_PLATFORM_TOOLSET_VERSION})
    # Display message only once in config mode
    if(NOT DEFINED Vcvars_BATCH_FILE)
      _vcvars_message(STATUS "Setting Vcvars_PLATFORM_TOOLSET_VERSION to '${Vcvars_PLATFORM_TOOLSET_VERSION}' as CMAKE_VS_PLATFORM_TOOLSET_VERSION was '${CMAKE_VS_PLATFORM_TOOLSET_VERSION}'")
    endif()
  elseif(DEFINED Vcvars_MSVC_VERSION)
    Vcvars_ConvertMsvcVersionToVcToolsetVersion(${Vcvars_MSVC_VERSION} Vcvars_PLATFORM_TOOLSET_VERSION)
    # Display message only once in config mode
    if(NOT DEFINED Vcvars_BATCH_FILE)
      _vcvars_message(STATUS "Setting Vcvars_PLATFORM_TOOLSET_VERSION to '${Vcvars_PLATFORM_TOOLSET_VERSION}' as Vcvars_MSVC_VERSION was '${Vcvars_MSVC_VERSION}'")
    endif()
  endif()
endif()

if(NOT DEFINED Vcvars_BATCH_FILE AND DEFINED Vcvars_MSVC_VERSION)
  Vcvars_FindFirstValidMsvcVersion(
    "${Vcvars_MSVC_ARCH}"
    "${Vcvars_MSVC_VERSION}"
    _msvc_version # Unused (Vcvars_MSVC_VERSION already set)
    _batch_file
    )
  if(_batch_file)
    Vcvars_ConvertMsvcVersionToVsVersion(${Vcvars_MSVC_VERSION} _vs_version)
    set(Vcvars_BATCH_FILE ${_batch_file}
      CACHE FILEPATH "Visual Studio ${_vs_version} vcvars script"
      )
    unset(_vs_version)
  endif()
  unset(_batch_file)
  unset(_msvc_version)
endif()

# configure wrapper script
if(Vcvars_BATCH_FILE)

  set(_vcvarsall_arch )
  set(_vcvarsall_vcvars_ver )
  if(Vcvars_FIND_VCVARSALL)
    if(Vcvars_MSVC_ARCH STREQUAL "64")
      set(_vcvarsall_arch "x64")
    elseif(Vcvars_MSVC_ARCH STREQUAL "32")
      set(_vcvarsall_arch "x86")
    endif()
    if(DEFINED Vcvars_PLATFORM_TOOLSET_VERSION)
      set(_vcvarsall_vcvars_ver "-vcvars_ver=${Vcvars_PLATFORM_TOOLSET_VERSION}")
    endif()
  endif()
  set(_in "${CMAKE_BINARY_DIR}/${CMAKE_FILES_DIRECTORY}/Vcvars_wrapper.bat.in")
  get_filename_component(_basename ${Vcvars_BATCH_FILE} NAME_WE)
  set(_out "${CMAKE_BINARY_DIR}/${CMAKE_FILES_DIRECTORY}/${_basename}_wrapper.bat")
  file(WRITE ${_in} "call \"@Vcvars_BATCH_FILE@\" @_vcvarsall_arch@ @_vcvarsall_vcvars_ver@
%*
")
  configure_file(${_in} ${_out} @ONLY)

  set(Vcvars_LAUNCHER ${_out})
  unset(_in)
  unset(_out)
  unset(_vcvarsall_arch)
else()
  set(Vcvars_LAUNCHER "Vcvars_LAUNCHER-NOTFOUND")
endif()


# outputs
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Vcvars
  FOUND_VAR Vcvars_FOUND
  REQUIRED_VARS
    Vcvars_BATCH_FILE
    Vcvars_LAUNCHER
    Vcvars_MSVC_VERSION
    Vcvars_MSVC_ARCH
    Vcvars_PLATFORM_TOOLSET_VERSION
  FAIL_MESSAGE
    "Failed to find vcvars scripts for Vcvars_MSVC_VERSION [${Vcvars_MSVC_VERSION}] and Vcvars_MSVC_ARCH [${Vcvars_MSVC_ARCH}]"
  )
