#include "stdafx.h"
#include "videodatabaseDirectory.h"
#include "../util.h"
#include "videodatabasedirectory/QueryParams.h"
#include "../VideoDatabase.h"
#include "TextureManager.h"

using namespace XFILE;
using namespace DIRECTORY;
using namespace VIDEODATABASEDIRECTORY;

CVideoDatabaseDirectory::CVideoDatabaseDirectory(void)
{
}

CVideoDatabaseDirectory::~CVideoDatabaseDirectory(void)
{
}

bool CVideoDatabaseDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  auto_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(strPath));

  if (!pNode.get())
    return false;

  bool bResult = pNode->GetChilds(items);
  for (int i=0;i<items.Size();++i)
  {
    if (items[i]->m_bIsFolder && !items[i]->HasThumbnail())
    {
      CStdString strImage = GetIcon(items[i]->m_strPath);
      if (g_TextureManager.Load(strImage))
      {
        items[i]->SetThumbnailImage(strImage);
        g_TextureManager.ReleaseTexture(strImage);
      }
    }
  }

  return bResult;
}

NODE_TYPE CVideoDatabaseDirectory::GetDirectoryChildType(const CStdString& strPath)
{
  auto_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(strPath));

  if (!pNode.get())
    return NODE_TYPE_NONE;

  return pNode->GetChildType();
}

NODE_TYPE CVideoDatabaseDirectory::GetDirectoryType(const CStdString& strPath)
{
  auto_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(strPath));

  if (!pNode.get())
    return NODE_TYPE_NONE;

  return pNode->GetType();
}

NODE_TYPE CVideoDatabaseDirectory::GetDirectoryParentType(const CStdString& strPath)
{
  auto_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(strPath));

  if (!pNode.get())
    return NODE_TYPE_NONE;

  CDirectoryNode* pParentNode=pNode->GetParent();

  if (!pParentNode)
    return NODE_TYPE_NONE;

  return pParentNode->GetChildType();
}

bool CVideoDatabaseDirectory::GetQueryParams(const CStdString& strPath, CQueryParams& params)
{
  auto_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(strPath));

  if (!pNode.get())
    false;
  
  CDirectoryNode::GetDatabaseInfo(strPath,params);
  return true;
}

void CVideoDatabaseDirectory::ClearDirectoryCache(const CStdString& strDirectory)
{
  CFileItem directory(strDirectory, true);
  if (CUtil::HasSlashAtEnd(directory.m_strPath))
    directory.m_strPath.Delete(directory.m_strPath.size() - 1);

  Crc32 crc;
  crc.ComputeFromLowerCase(directory.m_strPath);

  CStdString strFileName;
  strFileName.Format("Z:\\%08x.fi", crc);
  CFile::Delete(strFileName);
}

bool CVideoDatabaseDirectory::IsAllItem(const CStdString& strDirectory)
{
  if (strDirectory.Right(4).Equals("/-1/"))
    return true;
  return false;
}

bool CVideoDatabaseDirectory::GetLabel(const CStdString& strDirectory, CStdString& strLabel)
{
  strLabel = "";

  auto_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(strDirectory));
  if (!pNode.get() || strDirectory.IsEmpty())
    return false;

  // first see if there's any filter criteria
  CQueryParams params;
  CDirectoryNode::GetDatabaseInfo(strDirectory, params);

  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return false;

  // get genre
  CStdString strTemp;
  if (params.GetGenreId() != -1)
  {
    videodatabase.GetGenreById(params.GetGenreId(), strTemp);
    strLabel += strTemp;
  }

  // get year
  if (params.GetYear() != -1)
  { 
    strTemp.Format("%i",params.GetYear());   
    if (!strLabel.IsEmpty())
      strLabel += " / ";
    strLabel += strTemp;
  }

  if (strLabel.IsEmpty())
  {
    switch (pNode->GetChildType())
    {
    case NODE_TYPE_TITLE_MOVIES:
    case NODE_TYPE_TITLE_TVSHOWS:
    case NODE_TYPE_TITLE_MUSICVIDEOS:
      strLabel = g_localizeStrings.Get(369); break;
    case NODE_TYPE_ACTOR: // Actor
      strLabel = g_localizeStrings.Get(344); break;
    case NODE_TYPE_GENRE: // Genres
      strLabel = g_localizeStrings.Get(135); break;
    case NODE_TYPE_YEAR: // Year
      strLabel = g_localizeStrings.Get(345); break;
    case NODE_TYPE_DIRECTOR: // Director
      strLabel = g_localizeStrings.Get(20348); break;
    case NODE_TYPE_MOVIES_OVERVIEW: // Movies
      strLabel = g_localizeStrings.Get(342); break;
    case NODE_TYPE_TVSHOWS_OVERVIEW: // TV Shows
      strLabel = g_localizeStrings.Get(20343); break;
    case NODE_TYPE_RECENTLY_ADDED_MOVIES: // Recently Added Movies
      strLabel = g_localizeStrings.Get(20386); break;
    case NODE_TYPE_RECENTLY_ADDED_EPISODES: // Recently Added Episodes
      strLabel = g_localizeStrings.Get(20387); break;
    case NODE_TYPE_STUDIO: // Studios
      strLabel = g_localizeStrings.Get(20388); break;
    case NODE_TYPE_MUSICVIDEOS_OVERVIEW: // Music Videos
      strLabel = g_localizeStrings.Get(20389); break;
    case NODE_TYPE_RECENTLY_ADDED_MUSICVIDEOS: // Recently Added Music Videos
      strLabel = g_localizeStrings.Get(20390); break;
    default:
      CLog::Log(LOGWARNING, __FUNCTION__" - Unknown nodetype requested %d", pNode->GetChildType());
      return false;
    }
  }

  return true;
}

CStdString CVideoDatabaseDirectory::GetIcon(const CStdString& strDirectory)
{
  switch (GetDirectoryChildType(strDirectory))
  {
  case NODE_TYPE_TITLE_MOVIES:
    if (strDirectory.Equals("videodb://1/2/"))
      return "DefaultMovieTitle.png";
    return "";
  case NODE_TYPE_TITLE_TVSHOWS:
    if (strDirectory.Equals("videodb://2/2/"))
      return "DefaultTvshowTitle.png";
    return "";
  case NODE_TYPE_TITLE_MUSICVIDEOS:
    if (strDirectory.Equals("videodb://3/2/"))
      return "DefaultMusicVideoTitle.png";
    return "";
  case NODE_TYPE_ACTOR: // Actor
    return "DefaultActor.png";
  case NODE_TYPE_GENRE: // Genres
    return "DefaultGenre.png";
  case NODE_TYPE_YEAR: // Year
    return "DefaultYear.png";
  case NODE_TYPE_DIRECTOR: // Director
    return "DefaultDirector.png";
  case NODE_TYPE_MOVIES_OVERVIEW: // Movies
    return "DefaultMovies.png";
  case NODE_TYPE_TVSHOWS_OVERVIEW: // TV Shows
    return "DefaultTvshows.png";
  case NODE_TYPE_RECENTLY_ADDED_MOVIES: // Recently Added Movies
    return "DefaultRecentlyAddedMovies.png";
  case NODE_TYPE_RECENTLY_ADDED_EPISODES: // Recently Added Episodes
    return "DefaultRecentlyAddedEpisodes.png";
  case NODE_TYPE_RECENTLY_ADDED_MUSICVIDEOS: // Recently Added Episodes
    return "DefaultRecentlyAddedMusicVideos.png";
  case NODE_TYPE_STUDIO: // Studios
    return "DefaultStudios.png";
  case NODE_TYPE_MUSICVIDEOS_OVERVIEW: // Music Videos
    return "DefaultMusicVideos.png";
  default:
    CLog::Log(LOGWARNING, __FUNCTION__" - Unknown nodetype requested %s", strDirectory.c_str());
    break;
  }

  return "";
}

bool CVideoDatabaseDirectory::ContainsMovies(const CStdString &path)
{
  VIDEODATABASEDIRECTORY::NODE_TYPE type = GetDirectoryChildType(path);
  if (type == VIDEODATABASEDIRECTORY::NODE_TYPE_TITLE_MOVIES || type == VIDEODATABASEDIRECTORY::NODE_TYPE_EPISODES || type == VIDEODATABASEDIRECTORY::NODE_TYPE_TITLE_MUSICVIDEOS) return true;
  return false;
}

bool CVideoDatabaseDirectory::Exists(const char* strPath)
{
  auto_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(strPath));

  if (!pNode.get())
    return false;

  return true;
}

bool CVideoDatabaseDirectory::CanCache(const CStdString& strPath)
{
  auto_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(strPath));
  if (!pNode.get())
    return false;
  return pNode->CanCache();
}
