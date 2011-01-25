#pragma once
/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "utils/StdString.h"
#include "utils/Variant.h"

class CFileItemList;

namespace XFILE
{
  enum DIR_CACHE_TYPE
  {
    DIR_CACHE_NEVER = 0, ///< Never cache this directory to memory
    DIR_CACHE_ONCE,      ///< Cache this directory to memory for each fetch (so that FileExists() checks are fast)
    DIR_CACHE_ALWAYS     ///< Always cache this directory to memory, so that each additional fetch of this folder will utilize the cache (until it's cleared)
  };

/*!
 \ingroup filesystem
 \brief Interface to the directory on a file system.

 This Interface is retrieved from CFactoryDirectory and can be used to
 access the directories on a filesystem.
 \sa CFactoryDirectory
 */
class IDirectory
{
public:
  IDirectory(void);
  virtual ~IDirectory(void);
  /*!
   \brief Get the \e items of the directory \e strPath.
   \param strPath Directory to read.
   \param items Retrieves the directory entries.
   \return Returns \e true, if successfull.
   \sa CFactoryDirectory
   */
  virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items) = 0;
  /*!
  \brief Create the directory
  \param strPath Directory to create.
  \return Returns \e true, if directory is created or if it already exists
  \sa CFactoryDirectory
  */
  virtual bool Create(const char* strPath) { return false; }
  /*!
  \brief Check for directory existence
  \param strPath Directory to check.
  \return Returns \e true, if directory exists
  \sa CFactoryDirectory
  */
  virtual bool Exists(const char* strPath) { return false; }
  /*!
  \brief Removes the directory
  \param strPath Directory to remove.
  \return Returns \e false if not succesfull
  */
  virtual bool Remove(const char* strPath) { return false; }

  /*!
  \brief Whether this file should be listed
  \param strFile File to test.
  \return Returns \e true if the file should be listed
  */
  virtual bool IsAllowed(const CStdString& strFile) const;

  /*!
  \brief How this directory should be cached
  \param strPath Directory at hand.
  \return Returns the cache type.
  */
  virtual DIR_CACHE_TYPE GetCacheType(const CStdString& strPath) const { return DIR_CACHE_ONCE; };

  void SetMask(const CStdString& strMask);
  void SetAllowPrompting(bool allowPrompting);
  void SetCacheDirectory(DIR_CACHE_TYPE cacheDirectory);
  void SetUseFileDirectories(bool useFileDirectories);
  void SetExtFileInfo(bool extFileInfo);

  /*! \brief Process additional requirements before the directory fetch is performed.
   Some directory fetches may require authentication, keyboard input etc.  The IDirectory subclass
   should call GetKeyboardInput, SetErrorDialog or RequireAuthentication and then return false 
   from the GetDirectory method. CDirectory will then prompt for input from the user, before
   re-calling the GetDirectory method.
   \sa GetKeyboardInput, SetErrorDialog, RequireAuthentication
   */
  bool ProcessRequirements();

protected:
  /*! \brief Prompt the user for some keyboard input
   Call this method from the GetDirectory method to retrieve additional input from the user.
   If this function returns false then no input has been received, and the GetDirectory call
   should return false.
   \param heading an integer or string heading for the keyboard dialog
   \param input [out] the returned input (if available).
   \return true if keyboard input has been received. False if it hasn't.
   \sa ProcessRequirements
   */
  bool GetKeyboardInput(const CVariant &heading, CStdString &input);

  /*! \brief Show an error dialog on failure of GetDirectory call
   Call this method from the GetDirectory method to set an error message to be shown to the user
   \param heading an integer or string heading for the error dialog.
   \param line1 the first line to be displayed (integer or string).
   \param line2 the first line to be displayed (integer or string).
   \param line3 the first line to be displayed (integer or string).
   \sa ProcessRequirements
   */
  void SetErrorDialog(const CVariant &heading, const CVariant &line1, const CVariant &line2 = 0, const CVariant &line3 = 0);

  /*! \brief Prompt the user for authentication of a URL.
   Call this method from the GetDirectory method when authentication is required from the user, before returning
   false from the GetDirectory call. The user will be prompted for authentication, and GetDirectory will be
   re-called.
   \param url the URL to authenticate.
   \sa ProcessRequirements
   */
  void RequireAuthentication(const CStdString &url);

  CStdString m_strFileMask;  ///< Holds the file mask specified by SetMask()
  bool m_allowPrompting;    ///< If true, the directory handlers may prompt the user
  DIR_CACHE_TYPE m_cacheDirectory;    ///< If !DIR_CACHE_NEVER the directory is cached by g_directoryCache (defaults to DIR_CACHE_ONCE)
  bool m_useFileDirectories; ///< If true the directory may allow file directories (defaults to false)
  bool m_extFileInfo;       ///< If true the GetDirectory call can retrieve extra file information (defaults to true)

  CVariant m_requirements;
};
}
