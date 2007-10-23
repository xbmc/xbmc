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
#include "PictureInfoLoader.h"
#include "PictureInfoTag.h"

CPictureInfoLoader::CPictureInfoLoader()
{
}

CPictureInfoLoader::~CPictureInfoLoader()
{
}

void CPictureInfoLoader::OnLoaderStart()
{
  // Load previously cached items from HD
  m_mapFileItems.m_strPath=m_pVecItems->m_strPath;
  m_mapFileItems.Load();
  m_mapFileItems.SetFastLookup(true);

  m_tagReads = 0;
  m_loadTags = g_guiSettings.GetBool("pictures.usetags");

  if (m_pProgressCallback)
    m_pProgressCallback->SetProgressMax(m_pVecItems->GetFileCount());
}

bool CPictureInfoLoader::LoadItem(CFileItem* pItem)
{
  if (m_pProgressCallback && !pItem->m_bIsFolder)
    m_pProgressCallback->SetProgressAdvance();

  if (pItem->m_bIsFolder || pItem->IsZIP() || pItem->IsRAR() || pItem->IsCBR() || pItem->IsCBZ() || pItem->IsInternetStream())
    return false;

  if (pItem->HasPictureInfoTag())
    return true;

  CFileItem* mapItem=NULL;
  // first check the cached item
  if ((mapItem=m_mapFileItems[pItem->m_strPath])!=NULL && mapItem->m_dateTime==pItem->m_dateTime && mapItem->HasPictureInfoTag())
  { // Query map if we previously cached the file on HD
    *pItem->GetPictureInfoTag() = *mapItem->GetPictureInfoTag();
    pItem->SetThumbnailImage(mapItem->GetThumbnailImage());
    return true;
  }

  if (m_loadTags)
  { // Nothing found, load tag from file
    pItem->GetPictureInfoTag()->Load(pItem->m_strPath);
    m_tagReads++;
  }

  return true;
}

void CPictureInfoLoader::OnLoaderFinish()
{
  // cleanup cache loaded from HD
  m_mapFileItems.Clear();

  // Save loaded items to HD
  if (!m_bStop && m_tagReads > 0)
    m_pVecItems->Save();
}
