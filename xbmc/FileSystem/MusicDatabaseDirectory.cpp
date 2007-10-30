#include "stdafx.h"
#include "MusicDatabaseDirectory.h"
#include "../Util.h"
#include "MusicDatabaseDirectory/QueryParams.h"
#include "../MusicDatabase.h"

using namespace XFILE;
using namespace DIRECTORY;
using namespace MUSICDATABASEDIRECTORY;

CMusicDatabaseDirectory::CMusicDatabaseDirectory(void)
{
}

CMusicDatabaseDirectory::~CMusicDatabaseDirectory(void)
{
}

bool CMusicDatabaseDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  auto_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(strPath));

  if (!pNode.get())
    return false;

  return pNode->GetChilds(items);
}

NODE_TYPE CMusicDatabaseDirectory::GetDirectoryChildType(const CStdString& strPath)
{
  auto_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(strPath));

  if (!pNode.get())
    return NODE_TYPE_NONE;

  return pNode->GetChildType();
}

NODE_TYPE CMusicDatabaseDirectory::GetDirectoryType(const CStdString& strPath)
{
  auto_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(strPath));

  if (!pNode.get())
    return NODE_TYPE_NONE;

  return pNode->GetType();
}

NODE_TYPE CMusicDatabaseDirectory::GetDirectoryParentType(const CStdString& strPath)
{
  auto_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(strPath));

  if (!pNode.get())
    return NODE_TYPE_NONE;

  CDirectoryNode* pParentNode=pNode->GetParent();

  if (!pParentNode)
    return NODE_TYPE_NONE;

  return pParentNode->GetChildType();
}

bool CMusicDatabaseDirectory::IsArtistDir(const CStdString& strDirectory)
{
  NODE_TYPE node=GetDirectoryType(strDirectory);
  return (node==NODE_TYPE_ARTIST);
}

bool CMusicDatabaseDirectory::HasAlbumInfo(const CStdString& strDirectory)
{
  NODE_TYPE node=GetDirectoryType(strDirectory);
  return (node!=NODE_TYPE_OVERVIEW && node!=NODE_TYPE_TOP100 && 
          node!=NODE_TYPE_GENRE && node!=NODE_TYPE_ARTIST && node!=NODE_TYPE_YEAR);
}

void CMusicDatabaseDirectory::ClearDirectoryCache(const CStdString& strDirectory)
{
  CFileItem directory(strDirectory, true);
  if (CUtil::HasSlashAtEnd(directory.m_strPath))
    directory.m_strPath.Delete(directory.m_strPath.size() - 1);

  Crc32 crc;
  crc.ComputeFromLowerCase(directory.m_strPath);

  CStdString strFileName;
  strFileName.Format("Z:\\%08x.fi", (unsigned __int32) crc);
  CFile::Delete(strFileName);
}

bool CMusicDatabaseDirectory::IsAllItem(const CStdString& strDirectory)
{
  if (strDirectory.Right(4).Equals("/-1/"))
    return true;
  return false;
}

bool CMusicDatabaseDirectory::GetLabel(const CStdString& strDirectory, CStdString& strLabel)
{
  strLabel = "";

  auto_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(strDirectory));
  if (!pNode.get())
    return false;

  // first see if there's any filter criteria
  CQueryParams params;
  CDirectoryNode::GetDatabaseInfo(strDirectory, params);

  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return false;

  // get genre
  CStdString strTemp;
  if (params.GetGenreId() >= 0)
  {
    strTemp = "";
    musicdatabase.GetGenreById(params.GetGenreId(), strTemp);
    strLabel += strTemp;
  }

  // get artist
  if (params.GetArtistId() >= 0)
  {
    strTemp = "";
    musicdatabase.GetArtistById(params.GetArtistId(), strTemp);
    if (!strLabel.IsEmpty())
      strLabel += " / ";
    strLabel += strTemp;
  }

  // get album
  if (params.GetAlbumId() >= 0)
  {
    strTemp = "";
    musicdatabase.GetAlbumById(params.GetAlbumId(), strTemp);
    if (!strLabel.IsEmpty())
      strLabel += " / ";
    strLabel += strTemp;
  }

  if (strLabel.IsEmpty())
  {
    switch (pNode->GetChildType())
    {
    case NODE_TYPE_TOP100:
      strLabel = g_localizeStrings.Get(271); // Top 100
      break;
    case NODE_TYPE_GENRE:
      strLabel = g_localizeStrings.Get(135); // Genres
      break;
    case NODE_TYPE_ARTIST:
      strLabel = g_localizeStrings.Get(133); // Artists
      break;
    case NODE_TYPE_ALBUM:
      strLabel = g_localizeStrings.Get(132); // Albums
      break;
    case NODE_TYPE_ALBUM_RECENTLY_ADDED:
    case NODE_TYPE_ALBUM_RECENTLY_ADDED_SONGS:
      strLabel = g_localizeStrings.Get(359); // Recently Added Albums
      break;
    case NODE_TYPE_ALBUM_RECENTLY_PLAYED:
    case NODE_TYPE_ALBUM_RECENTLY_PLAYED_SONGS:
      strLabel = g_localizeStrings.Get(517); // Recently Played Albums
      break;
    case NODE_TYPE_ALBUM_TOP100:
    case NODE_TYPE_ALBUM_TOP100_SONGS:
      strLabel = g_localizeStrings.Get(10505); // Top 100 Albums
      break;
    case NODE_TYPE_SONG:
      strLabel = g_localizeStrings.Get(134); // Songs
      break;
    case NODE_TYPE_SONG_TOP100:
      strLabel = g_localizeStrings.Get(10504); // Top 100 Songs
      break;
    case NODE_TYPE_YEAR:
      strLabel = g_localizeStrings.Get(652);  // Years
      break;
    case NODE_TYPE_ALBUM_COMPILATIONS:
      strLabel = g_localizeStrings.Get(521);
      break;
    default:
      CLog::Log(LOGWARNING, __FUNCTION__" - Unknown nodetype requested %d", pNode->GetChildType());
      return false;
    }
  }

  return true;
}

bool CMusicDatabaseDirectory::ContainsSongs(const CStdString &path)
{
  MUSICDATABASEDIRECTORY::NODE_TYPE type = GetDirectoryChildType(path);
  if (type == MUSICDATABASEDIRECTORY::NODE_TYPE_SONG) return true;
  if (type == MUSICDATABASEDIRECTORY::NODE_TYPE_ALBUM_RECENTLY_ADDED_SONGS) return true;
  if (type == MUSICDATABASEDIRECTORY::NODE_TYPE_ALBUM_RECENTLY_PLAYED_SONGS) return true; 
  if (type == MUSICDATABASEDIRECTORY::NODE_TYPE_ALBUM_COMPILATIONS_SONGS) return true;
  if (type == MUSICDATABASEDIRECTORY::NODE_TYPE_ALBUM_TOP100_SONGS) return true;
  if (type == MUSICDATABASEDIRECTORY::NODE_TYPE_SONG_TOP100) return true;
  if (type == MUSICDATABASEDIRECTORY::NODE_TYPE_YEAR_SONG) return true;
  return false;
}

bool CMusicDatabaseDirectory::Exists(const char* strPath)
{
  auto_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(strPath));

  if (!pNode.get())
    return false;

  return true;
}

bool CMusicDatabaseDirectory::CanCache(const CStdString& strPath)
{
  auto_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(strPath));
  if (!pNode.get())
    return false;
  return pNode->CanCache();
}