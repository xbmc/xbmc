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

  static bool GetDirectory(const CStdString& strPath, CFileItemList &items, CStdString strMask="");
  static bool Create(const char* strPath);
  static bool Exists(const char* strPath);
  static bool Remove(const char* strPath);
};
}
