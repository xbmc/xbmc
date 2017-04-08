/*
 *      Copyright (C) 2012-2017 Team Kodi
 *      http://kodi.tv
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "XBDateTime.h"

#include <stdint.h>
#include <string>

class CVariant;

namespace GAME
{
  enum class SAVETYPE
  {
    UNKNOWN = 0,
    AUTO = 1,
    SLOT = 2,
    MANUAL = 3,
  };

  class CSavestate
  {
  public:
    CSavestate() { Reset(); }

    virtual ~CSavestate() = default;

    void Reset();

    const std::string& Path() const              { return m_path; }
    SAVETYPE           Type() const              { return m_type; }
    int                Slot() const              { return m_slot; }
    const std::string& Label() const             { return m_label; }
    size_t             Size() const              { return m_size; }
    const std::string& GameClient() const        { return m_gameClient; }
    int                DatabaseId() const        { return m_databaseId; }
    bool               IsDatabaseObject() const  { return m_databaseId != -1; }
    const std::string& GamePath() const          { return m_gamePath; }
    const std::string& GameCRC() const           { return m_gameCRC; }
    uint64_t           PlaytimeFrames() const    { return m_playtimeFrames; }
    double             PlaytimeWallClock() const { return m_playtimeWallClock; }
    const CDateTime&   Timestamp() const         { return m_timestamp; }
    const std::string& Thumbnail() const         { return m_thumbnail; }

    void SetPath(const std::string& path)             { m_path = path; }
    void SetType(SAVETYPE type)                       { m_type = type; }
    void SetSlot(int slot)                            { m_slot = slot; }
    void SetLabel(const std::string& label)           { m_label = label; }
    void SetSize(size_t size)                         { m_size = size; }
    void SetGameClient(const std::string& gameClient) { m_gameClient = gameClient; }
    void SetDatabaseId(int id)                        { m_databaseId = id; }
    void SetGamePath(const std::string& gamePath)     { m_gamePath = gamePath; }
    void SetGameCRC(const std::string& crc)           { m_gameCRC = crc; }
    void SetPlaytimeFrames(uint64_t frames)           { m_playtimeFrames = frames; }
    void SetPlaytimeWallClock(double playtime)        { m_playtimeWallClock = playtime; }
    void SetTimestamp(const CDateTime& timestamp)     { m_timestamp = timestamp; }
    void SetThumbnail(const std::string& thumbnail)   { m_thumbnail = thumbnail; }

    void Serialize(CVariant& value) const;
    void Deserialize(const CVariant& value);

    bool Serialize(const std::string& path) const;
    bool Deserialize(const std::string& path);

  private:
    // Savestate properties
    std::string  m_path;
    SAVETYPE     m_type;
    int          m_slot; // -1 for no slot
    std::string  m_label;
    size_t       m_size;
    std::string  m_gameClient;

    // Database properties
    int          m_databaseId;

    // Gameplay properties
    std::string  m_gamePath;
    std::string  m_gameCRC;
    uint64_t     m_playtimeFrames;
    double       m_playtimeWallClock; // seconds
    CDateTime    m_timestamp;
    std::string  m_thumbnail;
  };
}
