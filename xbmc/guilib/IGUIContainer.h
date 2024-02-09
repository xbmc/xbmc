/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GUIControl.h"

#include <memory>

/*!
 \ingroup controls
 \brief
 */

class IGUIContainer : public CGUIControl
{
protected:
  VIEW_TYPE m_type = VIEW_TYPE_NONE;
  std::string m_label;
public:
  IGUIContainer(int parentID, int controlID, float posX, float posY, float width, float height)
    : CGUIControl(parentID, controlID, posX, posY, width, height)
  {
  }

  bool IsContainer() const override { return true; }

  VIEW_TYPE GetType() const { return m_type; }
  const std::string& GetLabel() const { return m_label; }
  void SetType(VIEW_TYPE type, const std::string &label)
  {
    m_type = type;
    m_label = label;
  }

  virtual std::shared_ptr<CGUIListItem> GetListItem(int offset, unsigned int flag = 0) const = 0;
  virtual std::string GetLabel(int info) const                                 = 0;
};
