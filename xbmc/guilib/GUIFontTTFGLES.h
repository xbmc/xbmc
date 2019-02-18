/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GUIFontTTFGLBase.h"
#include "rendering/gles/RenderSystemGLES.h"

#include <string>

#include "system_gl.h"

class CGUIFontTTFGLES : public CGUIFontTTFGLBase
{
public:
  explicit CGUIFontTTFGLES(const std::string& strFileName);

  bool FirstBegin() override;
  void LastEnd() override;

private:
  CRenderSystemGLES* m_renderSystem = nullptr;
};

using CGUIFontTTF = CGUIFontTTFGLES;
