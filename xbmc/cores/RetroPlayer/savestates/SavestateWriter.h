/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "Savestate.h"
#include "SavestateDatabase.h"

#include <stdint.h>
#include <string>

namespace KODI
{
namespace GAME
{
  class CGameClient;
}

namespace RETRO
{
  class CSavestateWriter
  {
  public:
    ~CSavestateWriter();

    bool Initialize(const GAME::CGameClient* gameClient, uint64_t frameHistoryCount);
    bool WriteSave(const uint8_t *data, size_t size);
    void WriteThumb();
    bool CommitToDatabase();
    void CleanUpTransaction();
    const std::string& GetPath() const { return m_savestate.Path(); }

  private:
    CSavestate         m_savestate;
    double             m_fps = 0.0; //! @todo
    CSavestateDatabase m_db;
  };
}
}
