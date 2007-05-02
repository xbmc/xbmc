#include "stdafx.h"
#include "DirectoryNodeRoot.h"

using namespace DIRECTORY::VIDEODATABASEDIRECTORY;

CDirectoryNodeRoot::CDirectoryNodeRoot(const CStdString& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_ROOT, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeRoot::GetChildType()
{
  return NODE_TYPE_OVERVIEW;
}
