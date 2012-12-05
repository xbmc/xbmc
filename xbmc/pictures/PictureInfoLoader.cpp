/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "PictureInfoLoader.h"
#include "PictureInfoTag.h"
#include "settings/GUISettings.h"
#include "FileItem.h"

CPictureInfoLoader::CPictureInfoLoader()
{
  m_mapFileItems = new CFileItemList;
}

CPictureInfoLoader::~CPictureInfoLoader()
{
  StopThread();
  delete m_mapFileItems;
}

void CPictureInfoLoader::OnLoaderStart()
{
  // Load previously cached items from HD
  m_mapFileItems->SetPath(m_pVecItems->GetPath());
  m_mapFileItems->Load();
  m_mapFileItems->SetFastLookup(true);

  m_tagReads = 0;
  m_loadTags = g_guiSettings.GetBool("pictures.usetags");

  if (m_pProgressCallback)
    m_pProgressCallback->SetProgressMax(m_pVecItems->GetFileCount());
}

bool CPictureInfoLoader::LoadItem(CFileItem* pItem)
{
  if (m_pProgressCallback && !pItem->m_bIsFolder)
    m_pProgressCallback->SetProgressAdvance();

  if (!pItem->IsPicture() || pItem->IsZIP() || pItem->IsRAR() || pItem->IsCBR() || pItem->IsCBZ() || pItem->IsInternetStream() || pItem->IsVideo())
    return false;

  if (pItem->HasPictureInfoTag())
    return true;

  // first check the cached item
  CFileItemPtr mapItem = (*m_mapFileItems)[pItem->GetPath()];
  if (mapItem && mapItem->m_dateTime==pItem->m_dateTime && mapItem->HasPictureInfoTag())
  { // Query map if we previously cached the file on HD
    *pItem->GetPictureInfoTag() = *mapItem->GetPictureInfoTag();
    pItem->SetArt("thumb", mapItem->GetArt("thumb"));
    return true;
  }

  if (m_loadTags)
  { // Nothing found, load tag from file
    pItem->GetPictureInfoTag()->Load(pItem->GetPath());
    m_tagReads++;
  }

  return true;
}

void CPictureInfoLoader::OnLoaderFinish()
{
  // cleanup cache loaded from HD
  m_mapFileItems->Clear();

  // Save loaded items to HD
  if (!m_bStop && m_tagReads > 0)
    m_pVecItems->Save();
}
