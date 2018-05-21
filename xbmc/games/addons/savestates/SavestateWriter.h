/*
*      Copyright (C) 2016-present Team Kodi
*      http://kodi.tv
*
*  Kodi is free software: you can redistribute it and/or modify it
*  under the terms of the GNU General Public License as published by
*  the Free Software Foundation, either version 2 of the License, or
*  (at your option) any later version.
*
*  Kodi is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
*
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
    CSavestateWriter();
    ~CSavestateWriter();

    bool Initialize(const CGameClient* gameClient, uint64_t frameHistoryCount);
    bool WriteSave(IMemoryStream* memoryStream);
    void WriteThumb();
    bool CommitToDatabase();
    void CleanUpTransaction();
    const std::string& GetPath() const { return m_savestate.Path(); }

  private:
    CSavestate         m_savestate;
    double             m_fps; //! @todo
    CSavestateDatabase m_db;
  };
}
}
