#pragma once
#include "idirectory.h"

class CURL;
class TiXmlElement;

using namespace DIRECTORY;
namespace DIRECTORY
{
  class CDirectoryTuxBox : public IDirectory
  {
    public:
      CDirectoryTuxBox(void);
      virtual ~CDirectoryTuxBox(void);
      virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
  };
}