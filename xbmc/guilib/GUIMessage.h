/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GUIMacros.h"
#include "GUIMessageIDs.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

// forwards
class CGUIListItem;
class CFileItemList;

/*!
 \ingroup winmsg
 \brief
 */
class CGUIMessage final
{
public:
  CGUIMessage(int dwMsg, int senderID, int controlID, int64_t param1 = 0, int64_t param2 = 0);
  CGUIMessage(
      int msg, int senderID, int controlID, int64_t param1, int64_t param2, CFileItemList* item);
  CGUIMessage(int msg,
              int senderID,
              int controlID,
              int64_t param1,
              int64_t param2,
              const std::shared_ptr<CGUIListItem>& item);
  CGUIMessage(const CGUIMessage& msg);
  ~CGUIMessage(void);
  CGUIMessage& operator = (const CGUIMessage& msg);

  int GetControlId() const ;
  int GetMessage() const;
  void* GetPointer() const;
  std::shared_ptr<CGUIListItem> GetItem() const;
  int GetParam1() const;
  int64_t GetParam1AsI64() const;
  int GetParam2() const;
  int64_t GetParam2AsI64() const;
  int GetSenderId() const;
  void SetParam1(int64_t param1);
  void SetParam2(int64_t param2);
  void SetPointer(void* pointer);
  void SetLabel(const std::string& strLabel);
  void SetLabel(int iString);               // for convenience - looks up in strings.po
  const std::string& GetLabel() const;
  void SetStringParam(const std::string &strParam);
  void SetStringParams(const std::vector<std::string> &params);
  const std::string& GetStringParam(size_t param = 0) const;
  size_t GetNumStringParams() const;
  void SetItem(std::shared_ptr<CGUIListItem> item);

private:
  std::string m_strLabel;
  std::vector<std::string> m_params;
  int m_senderID;
  int m_controlID;
  int m_message;
  void* m_pointer;
  int64_t m_param1;
  int64_t m_param2;
  std::shared_ptr<CGUIListItem> m_item;

  static std::string empty_string;
};
