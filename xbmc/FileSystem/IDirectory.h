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


class CFileItemList;

namespace DIRECTORY
{
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

  bool IsAllowed(const CStdString& strFile);
  void SetMask(const CStdString& strMask);
  void SetAllowPrompting(bool allowPrompting);
  void SetCacheDirectory(bool cacheDirectory);
protected:
  CStdString m_strFileMask;  ///< Holds the file mask specified by SetMask()
  bool m_allowPrompting;    ///< If true, the directory handlers may prompt the user
  bool m_cacheDirectory;    ///< If true (default) the directory is cached by g_directoryCache.
};
}
