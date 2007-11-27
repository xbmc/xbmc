/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "PictureThumbLoader.h"
#include "Picture.h"
#include "Util.h"

using namespace XFILE;

CPictureThumbLoader::CPictureThumbLoader()
{
  m_regenerateThumbs = false;
  StopThread();
}

CPictureThumbLoader::~CPictureThumbLoader()
{
}

bool CPictureThumbLoader::LoadItem(CFileItem* pItem)
{
  if (pItem->m_bIsShareOrDrive) return true;
  pItem->SetCachedPictureThumb();
  
  if(pItem->HasThumbnail())
  {
    CStdString thumb(pItem->GetThumbnailImage());

    // look for remote thumbs    
    if (!CURL::IsFileOnly(thumb) && !CUtil::IsHD(thumb))
    {
      CStdString cachedThumb(pItem->GetCachedPictureThumb());
      if(CFile::Exists(cachedThumb))
        pItem->SetThumbnailImage(cachedThumb);
      else
      {
        CPicture pic;
        if(pic.DoCreateThumbnail(thumb, cachedThumb))
          pItem->SetThumbnailImage(cachedThumb);
        else
          pItem->SetThumbnailImage("");
      }
    }
    else if (m_regenerateThumbs)
    {
      CFile::Delete(thumb);
      pItem->SetThumbnailImage("");
    }
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