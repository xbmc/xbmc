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
  if (!pItem->HasThumbnail())
    pItem->SetUserVideoThumb();
  return true;
};
