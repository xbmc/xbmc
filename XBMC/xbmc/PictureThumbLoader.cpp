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
  pItem->SetThumb();
  if (m_regenerateThumbs && pItem->HasThumbnail())
  {
    CFile::Delete(pItem->GetThumbnailImage());
    pItem->SetThumbnailImage("");
  }
  if (pItem->IsPicture() && !pItem->HasThumbnail())
  { // load the thumb from the image file
    CPicture pic;
    pic.CreateThumbnail(pItem->m_strPath);
  }
  // refill in the icon to get it to update
  g_graphicsContext.Lock();
  pItem->FreeIcons();
  pItem->SetThumb();
  pItem->FillInDefaultIcon();
  g_graphicsContext.Unlock();
  return true;
};

void CPictureThumbLoader::OnLoaderFinish()
{
  m_regenerateThumbs = false;
}