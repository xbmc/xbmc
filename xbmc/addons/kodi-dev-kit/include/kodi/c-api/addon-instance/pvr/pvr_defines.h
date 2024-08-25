/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifndef C_API_ADDONINSTANCE_PVR_DEFINES_H
#define C_API_ADDONINSTANCE_PVR_DEFINES_H

//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
// "C" Standard PVR definitions
//
// Values related to all parts and not used direct on addon, are to define here.
//
#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  /*!
   * @brief "C" Representation of an integer value, including a description.
   */
  typedef struct PVR_ATTRIBUTE_INT_VALUE
  {
    int iValue;
    const char* strDescription;
  } PVR_ATTRIBUTE_INT_VALUE;

  /*!
   * @brief "C" Representation of a string value, including a description
   */
  typedef struct PVR_ATTRIBUTE_STRING_VALUE
  {
    const char* strValue;
    const char* strDescription;
  } PVR_ATTRIBUTE_STRING_VALUE;

  /*!
   * @brief "C" Representation of a named value.
   */
  typedef struct PVR_NAMED_VALUE
  {
    const char* strName;
    const char* strValue;
  } PVR_NAMED_VALUE;

  /*!
   * @brief Handle used to return data from the PVR add-on to CPVRClient
   */
  struct PVR_HANDLE_STRUCT
  {
    const void* callerAddress; /*!< address of the caller */
    void* dataAddress; /*!< address to store data in */
    int dataIdentifier; /*!< parameter to pass back when calling the callback */
  };
  typedef struct PVR_HANDLE_STRUCT* PVR_HANDLE;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !C_API_ADDONINSTANCE_PVR_DEFINES_H */
