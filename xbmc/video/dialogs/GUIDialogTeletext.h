/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIDialog.h"
#include "video/Teletext.h"

class CBaseTexture;

class CGUIDialogTeletext : public CGUIDialog
{
public:
  CGUIDialogTeletext(void);
  ~CGUIDialogTeletext(void) override;
  bool OnMessage(CGUIMessage& message) override;
  bool OnAction(const CAction& action) override;
  bool OnBack(int actionID) override;
  void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions) override;
  void Render() override;
  void OnInitWindow() override;
  void OnDeinitWindow(int nextWindowID) override;

protected:
  bool                m_bClose;           /* Close sendet, needed for fade out */
  CBaseTexture*       m_pTxtTexture;      /* Texture info class to render to screen */
  CRect               m_vertCoords;       /* Coordinates of teletext field on screen */
  CTeletextDecoder    m_TextDecoder;      /* Decoding class for teletext code */

private:
  void SetCoordinates();
};
