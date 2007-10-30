#pragma once
#include "IDirectory.h"

class CURL;
class TiXmlElement;

namespace DIRECTORY
{
  class CDirectoryTuxBox : public IDirectory
  {
    public:
      CDirectoryTuxBox(void);
      virtual ~CDirectoryTuxBox(void);
      virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
    private:
      bool GetRootAndChildString(const CStdString strPath, CStdString& strBQRequest, CStdString& strXMLRootString, CStdString& strXMLChildString );
      bool UpdateProgress(CGUIDialogProgress* dlgProgress, CStdString strLn1, CStdString strLn2, int iPercent, bool bCLose);
  };
}
