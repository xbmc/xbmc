/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

class IProgressCallback
{
public:
  virtual ~IProgressCallback() = default;
  virtual void SetProgressMax(int max)=0;
  virtual void SetProgressAdvance(int nSteps=1)=0;
  virtual bool Abort()=0;
};
