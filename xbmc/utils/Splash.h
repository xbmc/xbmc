#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include <string>
#include "threads/Thread.h"

class CGUITextLayout;
class CGUIImage;

class CSplash : public CThread
{
public:
  CSplash(const std::string& imageName);
  virtual ~CSplash();

  bool Start();
  void Stop();

  // In case you don't want to use another thread
  void Show();
  void Show(const std::string& message);
  void Hide();

private:
  virtual void Process();
  virtual void OnStartup();
  virtual void OnExit();

  float fade;
  std::string m_ImageName;

  CGUITextLayout* m_messageLayout;
  CGUIImage* m_image;
  bool m_layoutWasLoading;
#ifdef HAS_DX
  D3DGAMMARAMP newRamp;
  D3DGAMMARAMP oldRamp;

#endif
};
