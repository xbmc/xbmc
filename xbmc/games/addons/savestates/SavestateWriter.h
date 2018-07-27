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
  class IMemoryStream;

  class CSavestateWriter
  {
  public:
    ~CSavestateWriter();

    bool Initialize(const CGameClient* gameClient, uint64_t frameHistoryCount);
    bool WriteSave(IMemoryStream* memoryStream);
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
