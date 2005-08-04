#pragma once
#include "idirectory.h"
#include "../utils/http.h"

using namespace DIRECTORY;
namespace DIRECTORY
{
class CShoutcastDirectory :
      public IDirectory, public IRunnable
{
public:
  CShoutcastDirectory(void);
  virtual ~CShoutcastDirectory(void);
  virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
  bool DownloadPlaylists(CFileItemList &items);
  void CacheItems(CFileItemList &items);
  void LoadCachedItems(CFileItemList &items);
  bool IsCacheValid();
  virtual void Run();

protected:
  bool m_Downloaded;
  bool m_Error;
  CHTTP m_http;

  CStdString m_strSource;
  CStdString m_strDestination;
};
}
