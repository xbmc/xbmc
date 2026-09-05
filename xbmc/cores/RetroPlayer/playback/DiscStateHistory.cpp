/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DiscStateHistory.h"

#include "utils/log.h"

#include <algorithm>

using namespace KODI;
using namespace RETRO;

uint32_t CDiscStateHistory::Intern(const GAME::CGameClientDiscModel& model)
{
  const auto it = std::find(m_states.begin(), m_states.end(), model);
  if (it != m_states.end())
    return static_cast<uint32_t>(it - m_states.begin()) + 1;

  m_states.emplace_back(model);
  const uint32_t id = static_cast<uint32_t>(m_states.size());
  const int selectedSlot = model.GetSelectedDiscIndex().has_value()
                               ? static_cast<int>(*model.GetSelectedDiscIndex())
                               : -1;
  CLog::Log(LOGDEBUG,
            "RetroPlayer[DISC]: Interned rewind disc state {}: slots={} selected={} ejected={}", id,
            model.Size(), selectedSlot, model.IsEjected());
  return id;
}

const GAME::CGameClientDiscModel* CDiscStateHistory::Get(uint32_t id) const
{
  if (id == 0 || id > m_states.size())
    return nullptr;

  return &m_states[id - 1];
}

void CDiscStateHistory::Clear()
{
  m_states.clear();
}
