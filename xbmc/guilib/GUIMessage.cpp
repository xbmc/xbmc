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

#include "GUIMessage.h"
#include "LocalizeStrings.h"

using namespace std;

CStdString CGUIMessage::empty_string;

CGUIMessage::CGUIMessage(int msg, int senderID, int controlID, int param1, int param2)
{
  m_message = msg;
  m_senderID = senderID;
  m_controlID = controlID;
  m_param1 = param1;
  m_param2 = param2;
  m_pointer = NULL;
}

CGUIMessage::CGUIMessage(int msg, int senderID, int controlID, int param1, int param2, CFileItemList *item)
{
  m_message = msg;
  m_senderID = senderID;
  m_controlID = controlID;
  m_param1 = param1;
  m_param2 = param2;
  m_pointer = item;
}

CGUIMessage::CGUIMessage(int msg, int senderID, int controlID, int param1, int param2, const CGUIListItemPtr &item)
{
  m_message = msg;
  m_senderID = senderID;
  m_controlID = controlID;
  m_param1 = param1;
  m_param2 = param2;
  m_pointer = NULL;
  m_item = item;
}

CGUIMessage::CGUIMessage(const CGUIMessage& msg)
{
  *this = msg;
}

CGUIMessage::~CGUIMessage(void)
{}


int CGUIMessage::GetControlId() const
{
  return m_controlID;
}

int CGUIMessage::GetMessage() const
{
  return m_message;
}

void* CGUIMessage::GetPointer() const
{
  return m_pointer;
}

CGUIListItemPtr CGUIMessage::GetItem() const
{
  return m_item;
}

int CGUIMessage::GetParam1() const
{
  return m_param1;
}

int CGUIMessage::GetParam2() const
{
  return m_param2;
}

int CGUIMessage::GetSenderId() const
{
  return m_senderID;
}


const CGUIMessage& CGUIMessage::operator = (const CGUIMessage& msg)
{
  if (this == &msg) return * this;

  m_message = msg.m_message;
  m_controlID = msg.m_controlID;
  m_param1 = msg.m_param1;
  m_param2 = msg.m_param2;
  m_pointer = msg.m_pointer;
  m_strLabel = msg.m_strLabel;
  m_senderID = msg.m_senderID;
  m_params = msg.m_params;
  m_item = msg.m_item;
  return *this;
}


void CGUIMessage::SetParam1(int param1)
{
  m_param1 = param1;
}

void CGUIMessage::SetParam2(int param2)
{
  m_param2 = param2;
}

void CGUIMessage::SetPointer(void* lpVoid)
{
  m_pointer = lpVoid;
}

void CGUIMessage::SetLabel(const string& strLabel)
{
  m_strLabel = strLabel;
}

const string& CGUIMessage::GetLabel() const
{
  return m_strLabel;
}

void CGUIMessage::SetLabel(int iString)
{
  m_strLabel = g_localizeStrings.Get(iString);
}

void CGUIMessage::SetStringParam(const CStdString& strParam)
{
  m_params.clear();
  if (strParam.size())
    m_params.push_back(strParam);
}

void CGUIMessage::SetStringParams(const vector<CStdString> &params)
{
  m_params = params;
}

const CStdString& CGUIMessage::GetStringParam(size_t param) const
{
  if (param >= m_params.size())
    return empty_string;
  return m_params[param];
}

size_t CGUIMessage::GetNumStringParams() const
{
  return m_params.size();
}
