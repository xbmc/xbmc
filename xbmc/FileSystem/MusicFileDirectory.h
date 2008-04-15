#pragma once

#include "IFileDirectory.h"
#include "musicInfoTag.h"

namespace DIRECTORY
{
  class CMusicFileDirectory : public IFileDirectory
  {
    public:
      CMusicFileDirectory(void);
      virtual ~CMusicFileDirectory(void);
      virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
      virtual bool Exists(const char* strPath);
      virtual bool ContainsFiles(const CStdString& strPath);
    protected:
      virtual int GetTrackCount(const CStdString& strPath) = 0;
      CStdString m_strExt;
      MUSIC_INFO::CMusicInfoTag m_tag;
  };
}
