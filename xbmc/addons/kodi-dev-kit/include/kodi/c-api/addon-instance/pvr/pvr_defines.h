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
   * @brief API array sizes which are used for data exchange between
   * Kodi and addon.
   */
  ///@{
#define PVR_ADDON_NAME_STRING_LENGTH 1024
#define PVR_ADDON_URL_STRING_LENGTH 1024
#define PVR_ADDON_DESC_STRING_LENGTH 1024
#define PVR_ADDON_INPUT_FORMAT_STRING_LENGTH 32
#define PVR_ADDON_EDL_LENGTH 64
#define PVR_ADDON_TIMERTYPE_ARRAY_SIZE 32
#define PVR_ADDON_TIMERTYPE_VALUES_ARRAY_SIZE 512
#define PVR_ADDON_TIMERTYPE_VALUES_ARRAY_SIZE_SMALL 128
#define PVR_ADDON_TIMERTYPE_STRING_LENGTH 128
#define PVR_ADDON_ATTRIBUTE_DESC_LENGTH 128
#define PVR_ADDON_ATTRIBUTE_VALUES_ARRAY_SIZE 512
#define PVR_ADDON_DESCRAMBLE_INFO_STRING_LENGTH 64
#define PVR_ADDON_DATE_STRING_LENGTH 32
#define PVR_ADDON_COUNTRIES_STRING_LENGTH 128
#define PVR_ADDON_LANGUAGES_STRING_LENGTH 128
  ///@}

  /*!
   * @brief "C" Representation of a general attribute integer value.
   */
  typedef struct PVR_ATTRIBUTE_INT_VALUE
  {
    int iValue;
    char strDescription[PVR_ADDON_ATTRIBUTE_DESC_LENGTH];
  } PVR_ATTRIBUTE_INT_VALUE;

  /*!
   * @brief "C" Representation of a named value.
   */
  typedef struct PVR_NAMED_VALUE
  {
    char strName[PVR_ADDON_NAME_STRING_LENGTH];
    char strValue[PVR_ADDON_NAME_STRING_LENGTH];
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
