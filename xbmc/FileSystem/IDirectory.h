#pragma once
#include "../FileItem.h"

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
