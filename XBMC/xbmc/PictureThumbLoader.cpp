#include "stdafx.h"
#include "PictureThumbLoader.h"
#include "Picture.h"

CPictureThumbLoader::CPictureThumbLoader()
{
}

CPictureThumbLoader::~CPictureThumbLoader()
{
}

bool CPictureThumbLoader::LoadItem(CFileItem* pItem)
{
  pItem->SetThumb(true);  // sets thumb even if it has an icon
  if (!pItem->m_bIsFolder && (!pItem->HasThumbnail() || pItem->HasDefaultThumb()))
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
