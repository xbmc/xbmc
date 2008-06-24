/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "include.h"
#include "GUIMessage.h"
#include "LocalizeStrings.h"

using namespace std;

CGUIMessage::CGUIMessage(DWORD dwMsg, DWORD dwSenderID, DWORD dwControlID, DWORD dwParam1, DWORD dwParam2)
{
  m_dwMessage = dwMsg;
  m_dwSenderID = dwSenderID;
  m_dwControlID = dwControlID;
  m_dwParam1 = dwParam1;
  m_dwParam2 = dwParam2;
  m_lpVoid = NULL;
}

CGUIMessage::CGUIMessage(DWORD dwMsg, DWORD dwSenderID, DWORD dwControlID, DWORD dwParam1, DWORD dwParam2, CFileItemList *item)
{
  m_dwMessage = dwMsg;
  m_dwSenderID = dwSenderID;
  m_dwControlID = dwControlID;
  m_dwParam1 = dwParam1;
  m_dwParam2 = dwParam2;
  m_lpVoid = item;
}

CGUIMessage::CGUIMessage(DWORD dwMsg, DWORD dwSenderID, DWORD dwControlID, DWORD dwParam1, DWORD dwParam2, const CGUIListItemPtr &item)
{
  m_dwMessage = dwMsg;
  m_dwSenderID = dwSenderID;
  m_dwControlID = dwControlID;
  m_dwParam1 = dwParam1;
  m_dwParam2 = dwParam2;
  m_lpVoid = (void *)&item;
}

CGUIMessage::CGUIMessage(DWORD dwMsg, DWORD dwSenderID, DWORD dwControlID, DWORD dwParam1, DWORD dwParam2, CVisualisation* vis)
{
  m_dwMessage = dwMsg;
  m_dwSenderID = dwSenderID;
  m_dwControlID = dwControlID;
  m_dwParam1 = dwParam1;
  m_dwParam2 = dwParam2;
  m_lpVoid = vis;
}

//#ifdef HAS_KAI_VOICE
CGUIMessage::CGUIMessage(DWORD dwMsg, DWORD dwSenderID, DWORD dwControlID, DWORD dwParam1, DWORD dwParam2, CGUIList* list)
{
  m_dwMessage = dwMsg;
  m_dwSenderID = dwSenderID;
  m_dwControlID = dwControlID;
  m_dwParam1 = dwParam1;
  m_dwParam2 = dwParam2;
  m_lpVoid = list;
}
//#endif

CGUIMessage::CGUIMessage(const CGUIMessage& msg)
{
  *this = msg;
}

CGUIMessage::~CGUIMessage(void)
{}


DWORD CGUIMessage::GetControlId() const
{
  return m_dwControlID;
}

DWORD CGUIMessage::GetMessage() const
{
  return m_dwMessage;
}

void* CGUIMessage::GetLPVOID() const
{
  return m_lpVoid;
}

DWORD CGUIMessage::GetParam1() const
{
  return m_dwParam1;
}

DWORD CGUIMessage::GetParam2() const
{
  return m_dwParam2;
}

DWORD CGUIMessage::GetSenderId() const
{
  return m_dwSenderID;
}


const CGUIMessage& CGUIMessage::operator = (const CGUIMessage& msg)
{
  if (this == &msg) return * this;

  m_dwMessage = msg.m_dwMessage;
  m_dwControlID = msg.m_dwControlID;
  m_dwParam1 = msg.m_dwParam1;
  m_dwParam2 = msg.m_dwParam2;
  m_lpVoid = msg.m_lpVoid;
  m_strLabel = msg.m_strLabel;
  m_dwSenderID = msg.m_dwSenderID;
  m_strParam = msg.m_strParam;
  return *this;
}


void CGUIMessage::SetParam1(DWORD dwParam1)
{
  m_dwParam1 = dwParam1;
}

void CGUIMessage::SetParam2(DWORD dwParam2)
{
  m_dwParam2 = dwParam2;
}

void CGUIMessage::SetLPVOID(void* lpVoid)
{
  m_lpVoid = lpVoid;
}

void CGUIMessage::SetLabel(const string& wstrLabel)
{
  m_strLabel = wstrLabel;
}

const string& CGUIMessage::GetLabel() const
{
  return m_strLabel;
}

void CGUIMessage::SetLabel(int iString)
{
  m_strLabel = g_localizeStrings.Get(iString);
}

void CGUIMessage::SetStringParam(const string& strParam)
{
  m_strParam = strParam;
}

const string& CGUIMessage::GetStringParam() const
{
  return m_strParam;
}
