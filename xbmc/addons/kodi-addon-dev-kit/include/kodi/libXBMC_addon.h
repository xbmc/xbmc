/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <string>
#include <vector>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>

#include "versions.h"
#if defined(BUILD_KODI_ADDON)
#include "IFileTypes.h"
#else
#include "filesystem/IFileTypes.h"
#endif

struct VFSDirEntry;
struct __stat64;

#ifdef _WIN32                   // windows
#ifndef _SSIZE_T_DEFINED
typedef intptr_t      ssize_t;
#define _SSIZE_T_DEFINED
#endif // !_SSIZE_T_DEFINED

#if defined(BUILD_KODI_ADDON)
#include "p8-platform/windows/dlfcn-win32.h"
#endif
#else // windows
#include <dlfcn.h>              // linux+osx
#endif

#ifdef LOG_DEBUG
#undef LOG_DEBUG
#endif
#ifdef LOG_INFO
#undef LOG_INFO
#endif
#ifdef LOG_NOTICE
#undef LOG_NOTICE
#endif
#ifdef LOG_ERROR
#undef LOG_ERROR
#endif

typedef void* (*KODIAddOnLib_RegisterMe)(void *addonData);
typedef void (*KODIAddOnLib_UnRegisterMe)(void *addonData, void *cbTable);
typedef void* (*KODIAudioEngineLib_RegisterMe)(void *addonData);
typedef void (*KODIAudioEngineLib_UnRegisterMe)(void *addonData, void *cbTable);
typedef void* (*KODIGUILib_RegisterMe)(void *addonData);
typedef void (*KODIGUILib_UnRegisterMe)(void *addonData, void *cbTable);
typedef void* (*KODIPVRLib_RegisterMe)(void *addonData);
typedef void (*KODIPVRLib_UnRegisterMe)(void *addonData, void *cbTable);
typedef void* (*KODICodecLib_RegisterMe)(void *addonData);
typedef void (*KODICodecLib_UnRegisterMe)(void *addonData, void *cbTable);
typedef void* (*KODIINPUTSTREAMLib_RegisterMe)(void *addonData);
typedef void (*KODIINPUTSTREAMLib_UnRegisterMe)(void *addonData, void *cbTable);
typedef void* (*KODIPeripheralLib_RegisterMe)(void *addonData);
typedef void (*KODIPeripheralLib_UnRegisterMe)(void *addonData, void *cbTable);
typedef void* (*KODIGameLib_RegisterMe)(void *addonData);
typedef void (*KODIGameLib_UnRegisterMe)(void *addonData, void *cbTable);

typedef struct AddonCB
{
  const char* libBasePath;  ///< Never, never change this!!!
  void*       addonData;
  KODIAddOnLib_RegisterMe           AddOnLib_RegisterMe;
  KODIAddOnLib_UnRegisterMe         AddOnLib_UnRegisterMe;
  KODIAudioEngineLib_RegisterMe     AudioEngineLib_RegisterMe;
  KODIAudioEngineLib_UnRegisterMe   AudioEngineLib_UnRegisterMe;
  KODICodecLib_RegisterMe           CodecLib_RegisterMe;
  KODICodecLib_UnRegisterMe         CodecLib_UnRegisterMe;
  KODIGUILib_RegisterMe             GUILib_RegisterMe;
  KODIGUILib_UnRegisterMe           GUILib_UnRegisterMe;
  KODIPVRLib_RegisterMe             PVRLib_RegisterMe;
  KODIPVRLib_UnRegisterMe           PVRLib_UnRegisterMe;
  KODIINPUTSTREAMLib_RegisterMe     INPUTSTREAMLib_RegisterMe;
  KODIINPUTSTREAMLib_UnRegisterMe   INPUTSTREAMLib_UnRegisterMe;
  KODIPeripheralLib_RegisterMe      PeripheralLib_RegisterMe;
  KODIPeripheralLib_UnRegisterMe    PeripheralLib_UnRegisterMe;
  KODIGameLib_RegisterMe            GameLib_RegisterMe;
  KODIGameLib_UnRegisterMe          GameLib_UnRegisterMe;
} AddonCB;

namespace ADDON
{
  typedef enum addon_log
  {
    LOG_DEBUG,
    LOG_INFO,
    LOG_NOTICE,
    LOG_ERROR
  } addon_log_t;

  typedef enum queue_msg
  {
    QUEUE_INFO,
    QUEUE_WARNING,
    QUEUE_ERROR
  } queue_msg_t;
}

namespace KodiAPI
{
namespace AddOn
{
typedef struct CB_AddOn
{
  void (*Log)(void *addonData, const ADDON::addon_log_t loglevel, const char *msg);
  void (*QueueNotification)(void *addonData, const ADDON::queue_msg_t type, const char *msg);
  bool (*WakeOnLan)(const char* mac);
  bool (*GetSetting)(void *addonData, const char *settingName, void *settingValue);
  char* (*TranslateSpecialProtocol)(const char *strSource);
  char* (*UnknownToUTF8)(const char *sourceDest);
  char* (*GetLocalizedString)(const void* addonData, long dwCode);
  char* (*GetDVDMenuLanguage)(const void* addonData);
  void (*FreeString)(const void* addonData, char* str);
  void (*FreeStringArray)(const void* addonData, char** arr, int numElements);

  void* (*OpenFile)(const void* addonData, const char* strFileName, unsigned int flags);
  void* (*OpenFileForWrite)(const void* addonData, const char* strFileName, bool bOverWrite);
  ssize_t (*ReadFile)(const void* addonData, void* file, void* lpBuf, size_t uiBufSize);
  bool (*ReadFileString)(const void* addonData, void* file, char *szLine, int iLineLength);
  ssize_t (*WriteFile)(const void* addonData, void* file, const void* lpBuf, size_t uiBufSize);
  void (*FlushFile)(const void* addonData, void* file);
  int64_t (*SeekFile)(const void* addonData, void* file, int64_t iFilePosition, int iWhence);
  int (*TruncateFile)(const void* addonData, void* file, int64_t iSize);
  int64_t (*GetFilePosition)(const void* addonData, void* file);
  int64_t (*GetFileLength)(const void* addonData, void* file);
  double (*GetFileDownloadSpeed)(const void* addonData, void* file);
  void (*CloseFile)(const void* addonData, void* file);
  int (*GetFileChunkSize)(const void* addonData, void* file);
  bool (*FileExists)(const void* addonData, const char *strFileName, bool bUseCache);
  int (*StatFile)(const void* addonData, const char *strFileName, struct __stat64* buffer);
  char *(*GetFilePropertyValue)(const void* addonData, void* file, XFILE::FileProperty type, const char *name);
  char **(*GetFilePropertyValues)(const void* addonData, void* file, XFILE::FileProperty type, const char *name, int *numPorperties);
  bool (*DeleteFile)(const void* addonData, const char *strFileName);
  bool (*CanOpenDirectory)(const void* addonData, const char* strURL);
  bool (*CreateDirectory)(const void* addonData, const char *strPath);
  bool (*DirectoryExists)(const void* addonData, const char *strPath);
  bool (*RemoveDirectory)(const void* addonData, const char *strPath);
  bool (*GetDirectory)(const void* addonData, const char *strPath, const char* mask, VFSDirEntry** items, unsigned int* num_items);
  void (*FreeDirectory)(const void* addonData, VFSDirEntry* items, unsigned int num_items);
  void* (*CURLCreate)(const void* addonData, const char* strURL);
  bool (*CURLAddOption)(const void* addonData, void* file, XFILE::CURLOPTIONTYPE type, const char* name, const char * value);
  bool (*CURLOpen)(const void* addonData, void* file, unsigned int flags);
} CB_AddOnLib;

} /* namespace AddOn */
} /* namespace KodiAPI */

namespace ADDON
{
  class CHelper_libXBMC_addon
  {
  public:
    CHelper_libXBMC_addon()
    {
      m_Handle = nullptr;
      m_Callbacks = nullptr;
    }

    ~CHelper_libXBMC_addon()
    {
      if (m_Handle && m_Callbacks)
      {
        m_Handle->AddOnLib_UnRegisterMe(m_Handle->addonData, m_Callbacks);
      }
    }

    bool RegisterMe(void *handle)
    {
      m_Handle = static_cast<AddonCB*>(handle);
      if (m_Handle)
        m_Callbacks = (KodiAPI::AddOn::CB_AddOnLib*)m_Handle->AddOnLib_RegisterMe(m_Handle->addonData);
      if (!m_Callbacks)
        fprintf(stderr, "libXBMC_addon-ERROR: AddOnLib_RegisterMe can't get callback table from Kodi !!!\n");

      return m_Callbacks != nullptr;
    }

    /*!
     * @brief Add a message to XBMC's log.
     * @param loglevel The log level of the message.
     * @param format The format of the message to pass to XBMC.
     * @note This method uses limited buffer (16k) for the formatted output.
     * So data, which will not fit into it, will be silently discarded.
     */
    void Log(const addon_log_t loglevel, const char *format, ... )
    {
      char buffer[16384];
      static constexpr size_t len = sizeof (buffer) - 1;
      va_list args;
      va_start (args, format);
      vsnprintf (buffer, len, format, args);
      va_end (args);
      buffer[len] = '\0'; // to be sure it's null-terminated
      m_Callbacks->Log(m_Handle->addonData, loglevel, buffer);
    }

    /*!
     * @brief Get a settings value for this add-on.
     * @param settingName The name of the setting to get.
     * @param settingValue The value.
     * @return True if the settings was fetched successfully, false otherwise.
     */
    bool GetSetting(const char* settingName, void *settingValue)
    {
      return m_Callbacks->GetSetting(m_Handle->addonData, settingName, settingValue);
    }

    /*!
    * @brief Translates a special protocol folder.
    * @param source The file / folder to translate.
    * @return The string translated to resolved path. Must be freed by calling FreeString() when done.
    */
    char *TranslateSpecialProtocol(const char *source)
    {
      return m_Callbacks->TranslateSpecialProtocol(source);
    }

    /*!
     * @brief Queue a notification in the GUI.
     * @param type The message type.
     * @param format The format of the message to pass to display in XBMC.
     */
    void QueueNotification(const queue_msg_t type, const char *format, ... )
    {
      char buffer[16384];
      va_list args;
      va_start (args, format);
      vsprintf (buffer, format, args);
      va_end (args);
      m_Callbacks->QueueNotification(m_Handle->addonData, type, buffer);
    }

    /*!
     * @brief Send WakeOnLan magic packet.
     * @param mac Network address of the host to wake.
     * @return True if the magic packet was successfully sent, false otherwise.
     */
    bool WakeOnLan(const char* mac)
    {
      return m_Callbacks->WakeOnLan(mac);
    }

    /*!
     * @brief Translate a string with an unknown encoding to UTF8.
     * @param str The string to translate.
     * @return The string translated to UTF8. Must be freed by calling FreeString() when done.
     */
    char* UnknownToUTF8(const char* str)
    {
      return m_Callbacks->UnknownToUTF8(str);
    }

    /*!
     * @brief Get a localised message.
     * @param dwCode The code of the message to get.
     * @return The message. Must be freed by calling FreeString() when done.
     */
    char* GetLocalizedString(int dwCode)
    {
      return m_Callbacks->GetLocalizedString(m_Handle->addonData, dwCode);
    }

    /*!
     * @brief Get the DVD menu language.
     * @return The language. Must be freed by calling FreeString() when done.
     */
    char* GetDVDMenuLanguage()
    {
      return m_Callbacks->GetDVDMenuLanguage(m_Handle->addonData);
    }

    /*!
     * @brief Free the memory used by str
     * @param str The string to free
     */
    void FreeString(char* str)
    {
      m_Callbacks->FreeString(m_Handle->addonData, str);
    }

    /*!
     * @brief Free the memory used by arr including its elements
     * @param arr The string array to free
     * @param numElements The length of the array
     */
    void FreeStringArray(char** arr, int numElements)
    {
      m_Callbacks->FreeStringArray(m_Handle->addonData, arr, numElements);
    }

    /*!
     * @brief Open the file with filename via XBMC's CFile. Needs to be closed by calling CloseFile() when done.
     * @param strFileName The filename to open.
     * @param flags The flags to pass. Documented in XBMC's File.h
     * @return A handle for the file, or NULL if it couldn't be opened.
     */
    void* OpenFile(const char* strFileName, unsigned int flags)
    {
      return m_Callbacks->OpenFile(m_Handle->addonData, strFileName, flags);
    }

    /*!
     * @brief Open the file with filename via XBMC's CFile in write mode. Needs to be closed by calling CloseFile() when done.
     * @param strFileName The filename to open.
     * @param bOverWrite True to overwrite, false otherwise.
     * @return A handle for the file, or NULL if it couldn't be opened.
     */
    void* OpenFileForWrite(const char* strFileName, bool bOverWrite)
    {
      return m_Callbacks->OpenFileForWrite(m_Handle->addonData, strFileName, bOverWrite);
    }

    /*!
     * @brief Read from an open file.
     * @param file The file handle to read from.
     * @param lpBuf The buffer to store the data in.
     * @param uiBufSize The size of the buffer.
     * @return number of successfully read bytes if any bytes were read and stored in
     *         buffer, zero if no bytes are available to read (end of file was reached)
     *         or undetectable error occur, -1 in case of any explicit error
     */
    ssize_t ReadFile(void* file, void* lpBuf, size_t uiBufSize)
    {
      return m_Callbacks->ReadFile(m_Handle->addonData, file, lpBuf, uiBufSize);
    }

    /*!
     * @brief Read a string from an open file.
     * @param file The file handle to read from.
     * @param szLine The buffer to store the data in.
     * @param iLineLength The size of the buffer.
     * @return True when a line was read, false otherwise.
     */
    bool ReadFileString(void* file, char *szLine, int iLineLength)
    {
      return m_Callbacks->ReadFileString(m_Handle->addonData, file, szLine, iLineLength);
    }

    /*!
     * @brief Write to a file opened in write mode.
     * @param file The file handle to write to.
     * @param lpBuf The data to write.
     * @param uiBufSize Size of the data to write.
     * @return number of successfully written bytes if any bytes were written,
     *         zero if no bytes were written and no detectable error occur,
     *         -1 in case of any explicit error
     */
    ssize_t WriteFile(void* file, const void* lpBuf, size_t uiBufSize)
    {
      return m_Callbacks->WriteFile(m_Handle->addonData, file, lpBuf, uiBufSize);
    }

    /*!
     * @brief Flush buffered data.
     * @param file The file handle to flush the data for.
     */
    void FlushFile(void* file)
    {
       m_Callbacks->FlushFile(m_Handle->addonData, file);
    }

    /*!
     * @brief Seek in an open file.
     * @param file The file handle to see in.
     * @param iFilePosition The new position.
     * @param iWhence Seek argument. See stdio.h for possible values.
     * @return The new position.
     */
    int64_t SeekFile(void* file, int64_t iFilePosition, int iWhence)
    {
      return m_Callbacks->SeekFile(m_Handle->addonData, file, iFilePosition, iWhence);
    }

    /*!
     * @brief Truncate a file to the requested size.
     * @param file The file handle to truncate.
     * @param iSize The new max size.
     * @return New size?
     */
    int TruncateFile(void* file, int64_t iSize)
    {
      return m_Callbacks->TruncateFile(m_Handle->addonData, file, iSize);
    }

    /*!
     * @brief The current position in an open file.
     * @param file The file handle to get the position for.
     * @return The requested position.
     */
    int64_t GetFilePosition(void* file)
    {
      return m_Callbacks->GetFilePosition(m_Handle->addonData, file);
    }

    /*!
     * @brief Get the file size of an open file.
     * @param file The file to get the size for.
     * @return The requested size.
     */
    int64_t GetFileLength(void* file)
    {
      return m_Callbacks->GetFileLength(m_Handle->addonData, file);
    }

    /*!
    * @brief Get the download speed of an open file if available.
    * @param file The file to get the size for.
    * @return The download speed in seconds.
    */
    double GetFileDownloadSpeed(void* file)
    {
      return m_Callbacks->GetFileDownloadSpeed(m_Handle->addonData, file);
    }

    /*!
     * @brief Close an open file.
     * @param file The file handle to close.
     */
    void CloseFile(void* file)
    {
      m_Callbacks->CloseFile(m_Handle->addonData, file);
    }

    /*!
     * @brief Get the chunk size for an open file.
     * @param file the file handle to get the size for.
     * @return The requested size.
     */
    int GetFileChunkSize(void* file)
    {
      return m_Callbacks->GetFileChunkSize(m_Handle->addonData, file);
    }

    /*!
     * @brief Check if a file exists.
     * @param strFileName The filename to check.
     * @param bUseCache Check in file cache.
     * @return true if the file exists false otherwise.
     */
    bool FileExists(const char *strFileName, bool bUseCache)
    {
      return m_Callbacks->FileExists(m_Handle->addonData, strFileName, bUseCache);
    }

    /*!
     * @brief Reads file status.
     * @param strFileName The filename to read the status from.
     * @param buffer The file status is written into this buffer.
     * @return The file status was successfully read.
     */
    int StatFile(const char *strFileName, struct __stat64* buffer)
    {
      return m_Callbacks->StatFile(m_Handle->addonData, strFileName, buffer);
    }

    /*!
    * @brief Get a property from an open file.
    * @param file The file to get an property for
    * @param type Type of the requested property.
    * @param name Name of the requested property / can be null.
    * @return The value of the requested property, must be FreeString'ed.
    */
    char *GetFilePropertyValue(void* file, XFILE::FileProperty type, const char *name)
    {
      return m_Callbacks->GetFilePropertyValue(m_Handle->addonData, file, type, name);
    }

    /*!
    * @brief Get multiple property values from an open file.
    * @param file The file to get the property values for
    * @param type Type of the requested property.
    * @param name Name of the requested property / can be null.
    * @param numValues Number of property values returned.
    * @return List of values of the requested property, must be FreeStringArray'ed.
    */
    char **GetFilePropertyValues(void* file, XFILE::FileProperty type, const char *name, int *numValues)
    {
      return m_Callbacks->GetFilePropertyValues(m_Handle->addonData, file, type, name, numValues);
    }

    /*!
     * @brief Deletes a file.
     * @param strFileName The filename to delete.
     * @return The file was successfully deleted.
     */
    bool DeleteFile(const char *strFileName)
    {
      return m_Callbacks->DeleteFile(m_Handle->addonData, strFileName);
    }

    /*!
     * @brief Checks whether a directory can be opened.
     * @param strUrl The URL of the directory to check.
     * @return True when it can be opened, false otherwise.
     */
    bool CanOpenDirectory(const char* strUrl)
    {
      return m_Callbacks->CanOpenDirectory(m_Handle->addonData, strUrl);
    }

    /*!
     * @brief Creates a directory.
     * @param strPath Path to the directory.
     * @return True when it was created, false otherwise.
     */
    bool CreateDirectory(const char *strPath)
    {
      return m_Callbacks->CreateDirectory(m_Handle->addonData, strPath);
    }

    /*!
     * @brief Checks if a directory exists.
     * @param strPath Path to the directory.
     * @return True when it exists, false otherwise.
     */
    bool DirectoryExists(const char *strPath)
    {
      return m_Callbacks->DirectoryExists(m_Handle->addonData, strPath);
    }

    /*!
     * @brief Removes a directory.
     * @param strPath Path to the directory.
     * @return True when it was removed, false otherwise.
     */
    bool RemoveDirectory(const char *strPath)
    {
      return m_Callbacks->RemoveDirectory(m_Handle->addonData, strPath);
    }

    /*!
     * @brief Lists a directory.
     * @param strPath Path to the directory.
     * @param mask File mask
     * @param items The directory entries
     * @param num_items Number of entries in directory
     * @return True if listing was successful, false otherwise.
     */
    bool GetDirectory(const char *strPath, const char* mask, VFSDirEntry** items, unsigned int* num_items)
    {
      return m_Callbacks->GetDirectory(m_Handle->addonData, strPath, mask, items, num_items);
    }

    /*!
     * @brief Free a directory list
     * @param items The directory entries
     * @param num_items Number of entries in directory
     */
    void FreeDirectory(VFSDirEntry* items, unsigned int num_items)
    {
      m_Callbacks->FreeDirectory(m_Handle->addonData, items, num_items);
    }

    /*!
    * @brief Create a Curl representation
    * @param strURL the URL of the Type.
    */
    void* CURLCreate(const char* strURL)
    {
      return m_Callbacks->CURLCreate(m_Handle->addonData, strURL);
    }

    /*!
    * @brief Adds options to the curl file created with CURLCeate
    * @param file file pointer to the file returned by CURLCeate
    * @param type option type to set
    * @param name name of the option
    * @param value value of the option
    */
    bool CURLAddOption(void* file, XFILE::CURLOPTIONTYPE type, const char* name, const char * value)
    {
      return m_Callbacks->CURLAddOption(m_Handle->addonData, file, type, name, value);
    }

    /*!
    * @brief Opens the curl file created with CURLCeate
    * @param file file pointer to the file returned by CURLCeate
    * @param flags one or more bitwise or combinded flags form XFILE
    */
    bool CURLOpen(void* file, unsigned int flags)
    {
      return m_Callbacks->CURLOpen(m_Handle->addonData, file, flags);
    }

  private:
    AddonCB* m_Handle;
    KodiAPI::AddOn::CB_AddOnLib *m_Callbacks;
  };
};
