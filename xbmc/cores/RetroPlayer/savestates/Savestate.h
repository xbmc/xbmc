/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "XBDateTime.h"

#include <stdint.h>
#include <string>

class CVariant;

namespace KODI
{
namespace RETRO
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
}
