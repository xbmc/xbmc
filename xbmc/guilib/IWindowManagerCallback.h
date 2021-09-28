/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file IWindowManagerCallback.h
\brief
*/

/*!
 \ingroup winman
 \brief
 */
class IWindowManagerCallback
{
public:
  IWindowManagerCallback(void);
  virtual ~IWindowManagerCallback(void);

  virtual void FrameMove(bool processEvents, bool processGUI = true) = 0;
  virtual void Render() = 0;
  virtual void Process() = 0;
  virtual bool GetRenderGUI() const { return false; }
};
