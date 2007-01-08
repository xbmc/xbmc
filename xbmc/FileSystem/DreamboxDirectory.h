#pragma once
#include "idirectory.h"

class CURL;
class TiXmlElement;

using namespace DIRECTORY;
namespace DIRECTORY
{
  class CDreamboxDirectory : public IDirectory
  {
    public:
      CDreamboxDirectory(void);
      virtual ~CDreamboxDirectory(void);
      virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
  };
}