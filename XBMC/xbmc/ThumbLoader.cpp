#include "stdafx.h"
#include "ThumbLoader.h"

CThumbLoader::CThumbLoader()
{
}

CThumbLoader::~CThumbLoader()
{
}

bool CThumbLoader::LoadItem(CFileItem* pItem)
{
  if (pItem->m_bIsShareOrDrive) return true;
  pItem->SetThumb();
  pItem->FillInDefaultIcon();
  return true;
};
