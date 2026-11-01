/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "LutLoader.h"

#include <utility>

CLutLoader::CLutLoader(LoadFunction load, ApplyFunction apply)
  : m_load(std::move(load)),
    m_apply(std::move(apply))
{
}

void CLutLoader::Load(AVColorPrimaries primaries)
{
  int lutSize = 0;
  if (!m_load(primaries, lutSize, m_view))
    lutSize = 0;

  m_size = lutSize;
  m_apply(m_size, m_view.Get());
}
