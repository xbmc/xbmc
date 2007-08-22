#include "stdafx.h"
#include "VideoDatabaseDirectory.h"
#include "../Util.h"
#include "VideoDatabaseDirectory/QueryParams.h"
#include "../VideoDatabase.h"

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

  return pNode->GetChilds(items);
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
  strFileName.Format("Z:\\%08x.fi", (unsigned __int32) crc);
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
  if (!pNode.get())
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
    default:
      CLog::Log(LOGWARNING, CStdString(__FUNCTION__) + " - Unknown nodetype requested %d", pNode->GetChildType());
      return false;
    }
  }

  return true;
}

bool CVideoDatabaseDirectory::ContainsMovies(const CStdString &path)
{
  VIDEODATABASEDIRECTORY::NODE_TYPE type = GetDirectoryChildType(path);
  if (type == VIDEODATABASEDIRECTORY::NODE_TYPE_TITLE_MOVIES || type == VIDEODATABASEDIRECTORY::NODE_TYPE_EPISODES) return true;
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
