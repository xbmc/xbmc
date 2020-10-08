/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GUIFontTTFGLBase.h"

#include <string>
#include <vector>

#include "system_gl.h"

class CGUIFontTTFGL : public CGUIFontTTFGLBase
{
public:
  explicit CGUIFontTTFGL(const std::string& strFileName);

  bool FirstBegin() override;
  void LastEnd() override;

private:
  CRenderSystemGL* m_renderSystem = nullptr;
};
