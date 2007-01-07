#include "stdafx.h"
#include "PictureThumbLoader.h"
#include "Picture.h"


CPictureThumbLoader::CPictureThumbLoader()
{
  m_regenerateThumbs = false;
}

CPictureThumbLoader::~CPictureThumbLoader()
{
}

bool CPictureThumbLoader::LoadItem(CFileItem* pItem)
{
  if (pItem->m_bIsShareOrDrive) return true;
  pItem->SetCachedPictureThumb();
  if (m_regenerateThumbs && pItem->HasThumbnail())
  {
    CFile::Delete(pItem->GetThumbnailImage());
    pItem->SetThumbnailImage("");
  }
  if ((pItem->IsPicture() && !pItem->IsZIP() && !pItem->IsRAR() && !pItem->IsCBZ() && !pItem->IsCBR() && !pItem->IsPlayList()) && !pItem->HasThumbnail())
  { // load the thumb from the image file
    CPicture pic;
    pic.DoCreateThumbnail(pItem->m_strPath, pItem->GetCachedPictureThumb());
  }
  // refill in the thumb to get it to update
  pItem->SetCachedPictureThumb();
  pItem->FillInDefaultIcon();
  return true;
};

void CPictureThumbLoader::OnLoaderFinish()
{
  m_regenerateThumbs = false;
}