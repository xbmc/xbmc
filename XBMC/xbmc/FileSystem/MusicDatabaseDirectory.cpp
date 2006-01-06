#include "../stdafx.h"
#include "musicdatabaseDirectory.h"
#include "../util.h"

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
          node!=NODE_TYPE_GENRE && node!=NODE_TYPE_ARTIST);
}

void CMusicDatabaseDirectory::ClearDirectoryCache(const CStdString& strDirectory)
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

bool CMusicDatabaseDirectory::IsAllItem(const CStdString& strDirectory)
{
  if (strDirectory.Right(3).Equals("-1/"))
    return true;
  return false;
}
