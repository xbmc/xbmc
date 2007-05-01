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
#include "KaiItem.h"
#include "utils/KaiClient.h"
#include "utils/HTTP.h"
#include "Util.h"
#include "Picture.h"

using namespace XFILE;

CKaiItem::CKaiItem(CStdString& strLabel) : CGUIListExItem(strLabel)
{
  m_pAvatar = NULL;

  if (IsAvatarCached())
  {
    UseCachedAvatar();
  }
}

CKaiItem::~CKaiItem(void)
{
  CLog::Log(LOGDEBUG, "Destroying Kai Item at %p", this);
  // remove any pending requests from this item
  g_DownloadManager.CancelRequests(this);
  if (m_pAvatar)
  {
    m_pAvatar->FreeResources();
    delete m_pAvatar;
    m_pAvatar = NULL;
  }
}

void CKaiItem::AllocResources()
{
  CGUIListExItem::AllocResources();

  if (m_pAvatar)
  {
    m_pAvatar->AllocResources();
  }
}

void CKaiItem::FreeResources()
{
  CGUIListExItem::FreeResources();

  if (m_pAvatar)
  {
    m_pAvatar->FreeResources();
  }
}

void CKaiItem::SetAvatar(CStdString& aAvatarUrl)
{
  // OutputDebugString("Setting Kai avatar\r\n");

  // Get file extension from url
  CStdString strExtension;
  CUtil::GetExtension(aAvatarUrl, strExtension);
  if (!strExtension.IsEmpty())
  {
    if (IsAvatarCached())
    {
      CStdString strAvatarFilePath;
      GetAvatarFilePath(strAvatarFilePath);

      if (m_pAvatar)
      {
        m_pAvatar->FreeResources();
        delete m_pAvatar;
      }
      m_pAvatar = new CGUIImage(0, 0, 0, 0, 50, 50, strAvatarFilePath);
    }
    else
    {
      g_DownloadManager.RequestFile(aAvatarUrl, this);
    }
  }
}

void CKaiItem::UseCachedAvatar()
{
  CStdString strAvatarFilePath;
  GetAvatarFilePath(strAvatarFilePath);

  if (m_pAvatar)
  {
    m_pAvatar->FreeResources();
    delete m_pAvatar;
  }
  m_pAvatar = new CGUIImage(0, 0, 0, 0, 50, 50, strAvatarFilePath);
}

bool CKaiItem::IsAvatarCached()
{
  // Compute cached filename
  CStdString strFilePath;
  GetAvatarFilePath(strFilePath);

  // Check to see if the file is already cached
  return CFile::Exists(strFilePath);
}

void CKaiItem::GetAvatarFilePath(CStdString& aFilePath)
{
  // Generate a unique identifier from player name
  Crc32 crc;
  crc.Compute(m_strName);
  aFilePath.Format("%s\\avatar-%x.jpg", g_settings.GetXLinkKaiThumbFolder().c_str(), (unsigned __int32) crc);
}

void CKaiItem::OnFileComplete(TICKET aTicket, CStdString& aFilePath, INT aByteRxCount, Result aResult)
{
  if (aResult == IDownloadQueueObserver::Succeeded && aByteRxCount >= 100)
  {
    //OutputDebugString("Downloaded avatar.\r\n");

    CStdString strAvatarFilePath;
    GetAvatarFilePath(strAvatarFilePath);

    try
    {
      if (!CFile::Exists(strAvatarFilePath))
      { // download and convert image
        CPicture picture;
        picture.DoCreateThumbnail(aFilePath, strAvatarFilePath);
      }
//      g_graphicsContext.Lock();
      if (!m_pAvatar)
        m_pAvatar = new CGUIImage(0, 0, 0, 0, 50, 50, strAvatarFilePath);
      if (m_pAvatar)
        m_pAvatar->SetFileName(strAvatarFilePath);
//      g_graphicsContext.Unlock();
    }
    catch (...)
    {
      ::DeleteFile(strAvatarFilePath.c_str());
    }
  }
  else
  {
    //OutputDebugString("Unable to download avatar.\r\n");
  }

  ::DeleteFile(aFilePath.c_str());
}

