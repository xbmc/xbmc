/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIMessage.h"

#include "LocalizeStrings.h"

std::string CGUIMessage::empty_string;

CGUIMessage::CGUIMessage(int msg, int senderID, int controlID, int64_t param1, int64_t param2)
{
  m_message = msg;
  m_senderID = senderID;
  m_controlID = controlID;
  m_param1 = param1;
  m_param2 = param2;
  m_pointer = NULL;
}

CGUIMessage::CGUIMessage(
    int msg, int senderID, int controlID, int64_t param1, int64_t param2, CFileItemList* item)
{
  m_message = msg;
  m_senderID = senderID;
  m_controlID = controlID;
  m_param1 = param1;
  m_param2 = param2;
  m_pointer = item;
}

CGUIMessage::CGUIMessage(int msg,
                         int senderID,
                         int controlID,
                         int64_t param1,
                         int64_t param2,
                         const std::shared_ptr<CGUIListItem>& item)
  : m_item(item)
{
  m_message = msg;
  m_senderID = senderID;
  m_controlID = controlID;
  m_param1 = param1;
  m_param2 = param2;
  m_pointer = NULL;
}

CGUIMessage::CGUIMessage(const CGUIMessage& msg) = default;

CGUIMessage::~CGUIMessage(void) = default;


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

std::shared_ptr<CGUIListItem> CGUIMessage::GetItem() const
{
  return m_item;
}

int CGUIMessage::GetParam1() const
{
  return static_cast<int>(m_param1);
}

int64_t CGUIMessage::GetParam1AsI64() const
{
  return m_param1;
}

int CGUIMessage::GetParam2() const
{
  return static_cast<int>(m_param2);
}

int64_t CGUIMessage::GetParam2AsI64() const
{
  return m_param2;
}

int CGUIMessage::GetSenderId() const
{
  return m_senderID;
}

CGUIMessage& CGUIMessage::operator = (const CGUIMessage& msg) = default;

void CGUIMessage::SetParam1(int64_t param1)
{
  m_param1 = param1;
}

void CGUIMessage::SetParam2(int64_t param2)
{
  m_param2 = param2;
}

void CGUIMessage::SetPointer(void* lpVoid)
{
  m_pointer = lpVoid;
}

void CGUIMessage::SetLabel(const std::string& strLabel)
{
  m_strLabel = strLabel;
}

const std::string& CGUIMessage::GetLabel() const
{
  return m_strLabel;
}

void CGUIMessage::SetLabel(int iString)
{
  m_strLabel = g_localizeStrings.Get(iString);
}

void CGUIMessage::SetStringParam(const std::string& strParam)
{
  m_params.clear();
  if (strParam.size())
    m_params.push_back(strParam);
}

void CGUIMessage::SetStringParams(const std::vector<std::string> &params)
{
  m_params = params;
}

const std::string& CGUIMessage::GetStringParam(size_t param) const
{
  if (param >= m_params.size())
    return empty_string;
  return m_params[param];
}

size_t CGUIMessage::GetNumStringParams() const
{
  return m_params.size();
}

void CGUIMessage::SetItem(std::shared_ptr<CGUIListItem> item)
{
  m_item = std::move(item);
}
