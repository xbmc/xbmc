#pragma once
#include "IDirectory.h"
#include "../utils/HTTP.h"

namespace DIRECTORY
{
class CLastFMDirectory :
      public IDirectory, public IRunnable
{
public:
  CLastFMDirectory(void);
  virtual ~CLastFMDirectory(void);
  virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
  virtual void Run();

protected:
  void AddEntry(int iString, CStdString strPath, CStdString strIconPath, bool bFolder, CFileItemList &items);
  void AddListEntry(const char *name, const char *artist, const char *count, const char *date, const char *icon, CStdString strPath, CFileItemList &items);
  CStdString BuildURLFromInfo();
  bool RetrieveList(CStdString url);
  bool ParseArtistList(CStdString url, CFileItemList &items);
  bool ParseAlbumList(CStdString url, CFileItemList &items);
  bool ParseUserList(CStdString url, CFileItemList &items);
  bool ParseTagList(CStdString url, CFileItemList &items);
  bool ParseTrackList(CStdString url, CFileItemList &items);

  bool GetArtistInfo(CFileItemList &items);
  bool GetUserInfo(CFileItemList &items);
  bool GetTagInfo(CFileItemList &items);

  bool SearchSimilarTags(CFileItemList &items);
  bool SearchSimilarArtists(CFileItemList &items);

  bool m_Error;
  bool m_Downloaded;
  CHTTP m_http;
  TiXmlDocument m_xmlDoc;

  CStdString m_objtype;
  CStdString m_objname;
  CStdString m_encodedobjname;
  CStdString m_objrequest;

  CStdString m_strSource;
  CStdString m_strDestination;

  CGUIDialogProgress* m_dlgProgress;
  CFileItemList m_vecCachedItems;
};
}
