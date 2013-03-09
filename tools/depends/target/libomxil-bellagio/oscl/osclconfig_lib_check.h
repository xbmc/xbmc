/* ------------------------------------------------------------------
 * Copyright (C) 1998-2009 PacketVideo
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 * -------------------------------------------------------------------
 */

#ifndef OSCLCONFIG_LIB_CHECK_H_INCLUDED
#define OSCLCONFIG_LIB_CHECK_H_INCLUDED

/*! \addtogroup osclconfig OSCL config
 *
 * @{
 */



/**
OSCL_HAS_RUNTIME_LIB_LOADING_SUPPORT should be set to 1 if the platform has basic explicit runtime DLL loading support.
*/
#if !defined(OSCL_HAS_RUNTIME_LIB_LOADING_SUPPORT )
#error "ERROR: OSCL_HAS_RUNTIME_LIB_LOADING_SUPPORT must be defined to 0 or 1"
#endif

#if(OSCL_HAS_RUNTIME_LIB_LOADING_SUPPORT)
/**
** When OSCL_HAS_RUNTIME_LIB_LOADING_SUPPORT is 1,
** OSCL_LIB_READ_DEBUG_LIBS should be set to 0 or 1.  Set to 1 to enable loading
** debug versions of libs.
*/
#if !defined(OSCL_LIB_READ_DEBUG_LIBS)
#error "ERROR: OSCL_LIB_READ_DEBUG_LIBS must be defined to 0 or 1"
#endif

/*
** When OSCL_HAS_RUNTIME_LIB_LOADING_SUPPORT is 1,
** PV_DYNAMIC_LOADING_CONFIG_FILE_PATH should be set.
*/
#if !defined(PV_DYNAMIC_LOADING_CONFIG_FILE_PATH)
#error "ERROR: PV_DYNAMIC_LOADING_CONFIG_FILE_PATH must be set to a path where the config files are expected to be present"
#endif

/*
** When OSCL_HAS_RUNTIME_LIB_LOADING_SUPPORT is 1,
** PV_RUNTIME_LIB_FILENAME_EXTENSION should be set.
*/
#if !defined(PV_RUNTIME_LIB_FILENAME_EXTENSION)
#error "ERROR: PV_RUNTIME_LIB_FILENAME_EXTENSION must be specified for use as the dynamic library file extension"
#endif
#endif // OSCL_HAS_RUNTIME_LIB_LOADING_SUPPORT

/*! @} */

#endif // OSCLCONFIG_LIB_CHECK_H_INCLUDED


