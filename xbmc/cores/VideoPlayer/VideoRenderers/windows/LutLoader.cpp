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
  m_task = Concurrency::create_task([load = m_load, primaries] { return load(primaries); });
}

void CLutLoader::ApplyFinishedLoad()
{
  if (!m_task || !m_task->is_done())
    return;

  const Lut lut = m_task->get();
  m_task.reset();
  m_apply(lut);
}
