#pragma once
#include "../FileItem.h"

namespace DIRECTORY
{
/*!
 \ingroup filesystem 
 \brief Wrappers for \e IDirectory
 */
class CDirectory
{
public:
  CDirectory(void);
  virtual ~CDirectory(void);

  static bool GetDirectory(const CStdString& strPath, CFileItemList &items, CStdString strMask="", bool bUseFileDirectories=true, bool allowPrompting=false);
  static bool Create(const CStdString& strPath);
  static bool Exists(const CStdString& strPath);
  static bool Remove(const CStdString& strPath);
};
}
