/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SavestateFlatBuffer.h"

#include "savestate_generated.h"
#include "utils/log.h"

using namespace KODI;
using namespace RETRO;

namespace
{
  const uint8_t SCHEMA_VERSION = 1;

  /*!
   * \brief The initial size of the FlatBuffer's memory buffer
   *
   * 1024 is the default size in the FlatBuffers header. We might as well use
   * this until our size requirements are more known.
   */
  const size_t INITIAL_FLATBUFFER_SIZE = 1024;

  /*!
   * \brief Translate the save type (RetroPlayer to FlatBuffers)
   */
  SaveType TranslateType(SAVE_TYPE type)
  {
    switch (type)
    {
    case SAVE_TYPE::AUTO:
      return SaveType_Auto;
    case SAVE_TYPE::MANUAL:
      return SaveType_Manual;
    default:
      break;
    }

    return SaveType_Unknown;
  }

  /*!
   * \brief Translate the save type (FlatBuffers to RetroPlayer)
   */
  SAVE_TYPE TranslateType(SaveType type)
  {
    switch (type)
    {
    case SaveType_Auto:
      return SAVE_TYPE::AUTO;
    case SaveType_Manual:
      return SAVE_TYPE::MANUAL;
    default:
      break;
    }

    return SAVE_TYPE::UNKNOWN;
  }
}

CSavestateFlatBuffer::CSavestateFlatBuffer()
{
  Reset();
}

CSavestateFlatBuffer::~CSavestateFlatBuffer() = default;

void CSavestateFlatBuffer::Reset()
{
  m_builder.reset(new flatbuffers::FlatBufferBuilder(INITIAL_FLATBUFFER_SIZE));
  m_data.clear();
  m_savestate = nullptr;
}

bool CSavestateFlatBuffer::Serialize(const uint8_t *&data, size_t &size) const
{
  // Check if savestate was deserialized from vector or built with FlatBuffers
  if (!m_data.empty())
  {
    data = m_data.data();
    size = m_data.size();
  }
  else
  {
    data = m_builder->GetBufferPointer();
    size = m_builder->GetSize();
  }

  return true;
}

SAVE_TYPE CSavestateFlatBuffer::Type() const
{
  if (m_savestate != nullptr)
    return TranslateType(m_savestate->type());

  return SAVE_TYPE::UNKNOWN;
}

void CSavestateFlatBuffer::SetType(SAVE_TYPE type)
{
  m_type = type;
}

uint8_t CSavestateFlatBuffer::Slot() const
{
  if (m_savestate != nullptr)
    return m_savestate->slot();

  return 0;
}

void CSavestateFlatBuffer::SetSlot(uint8_t slot)
{
  m_slot = slot;
}

std::string CSavestateFlatBuffer::Label() const
{
  std::string label;

  if (m_savestate != nullptr && m_savestate->label())
    label = m_savestate->label()->c_str();

  return label;
}

void CSavestateFlatBuffer::SetLabel(const std::string &label)
{
  m_labelOffset.reset(new StringOffset{ m_builder->CreateString(label) });
}

CDateTime CSavestateFlatBuffer::Created() const
{
  CDateTime created;

  if (m_savestate != nullptr && m_savestate->created())
    created.SetFromRFC1123DateTime(m_savestate->created()->c_str());

  return created;
}

void CSavestateFlatBuffer::SetCreated(const CDateTime &created)
{
  m_createdOffset.reset(new StringOffset{ m_builder->CreateString(created.GetAsRFC1123DateTime()) });
}

std::string CSavestateFlatBuffer::GameFileName() const
{
  std::string gameFileName;

  if (m_savestate != nullptr && m_savestate->game_file_name())
    gameFileName = m_savestate->game_file_name()->c_str();

  return gameFileName;
}

void CSavestateFlatBuffer::SetGameFileName(const std::string &gameFileName)
{
  m_gameFileNameOffset.reset(new StringOffset{ m_builder->CreateString(gameFileName) });
}

uint64_t CSavestateFlatBuffer::TimestampFrames() const
{
  return m_savestate->timestamp_frames();
}

void CSavestateFlatBuffer::SetTimestampFrames(uint64_t timestampFrames)
{
  m_timestampFrames = timestampFrames;
}

double CSavestateFlatBuffer::TimestampWallClock() const
{
  if (m_savestate != nullptr)
    return static_cast<double>(m_savestate->timestamp_wall_clock_ns()) / 1000.0 / 1000.0 / 1000.0;

  return 0.0;
}

void CSavestateFlatBuffer::SetTimestampWallClock(double timestampWallClock)
{
  m_timestampWallClock = timestampWallClock;
}

std::string CSavestateFlatBuffer::GameClientID() const
{
  std::string gameClientId;

  if (m_savestate != nullptr && m_savestate->emulator_addon_id())
    gameClientId = m_savestate->emulator_addon_id()->c_str();

  return gameClientId;
}

void CSavestateFlatBuffer::SetGameClientID(const std::string &gameClientId)
{
  m_emulatorAddonIdOffset.reset(new StringOffset{ m_builder->CreateString(gameClientId) });
}

std::string CSavestateFlatBuffer::GameClientVersion() const
{
  std::string gameClientVersion;

  if (m_savestate != nullptr && m_savestate->emulator_version())
    gameClientVersion = m_savestate->emulator_version()->c_str();

  return gameClientVersion;
}

void CSavestateFlatBuffer::SetGameClientVersion(const std::string &gameClientVersion)
{
  m_emulatorVersionOffset.reset(new StringOffset{ m_builder->CreateString(gameClientVersion) });
}

const uint8_t *CSavestateFlatBuffer::GetMemoryData() const
{
  if (m_savestate != nullptr && m_savestate->memory_data())
    return m_savestate->memory_data()->data();

  return nullptr;
}

size_t CSavestateFlatBuffer::GetMemorySize() const
{
  if (m_savestate != nullptr && m_savestate->memory_data())
    return m_savestate->memory_data()->size();

  return 0;
}

uint8_t *CSavestateFlatBuffer::GetMemoryBuffer(size_t size)
{
  uint8_t *memoryBuffer = nullptr;

  m_memoryDataOffset.reset(new VectorOffset{ m_builder->CreateUninitializedVector(size, &memoryBuffer) });

  return memoryBuffer;
}

void CSavestateFlatBuffer::Finalize()
{
  // Helper class to build the nested Savestate table
  SavestateBuilder savestateBuilder(*m_builder);

  savestateBuilder.add_version(SCHEMA_VERSION);

  savestateBuilder.add_type(TranslateType(m_type));

  savestateBuilder.add_slot(m_slot);

  if (m_labelOffset)
  {
    savestateBuilder.add_label(*m_labelOffset);
    m_labelOffset.reset();
  }

  if (m_createdOffset)
  {
    savestateBuilder.add_created(*m_createdOffset);
    m_createdOffset.reset();
  }

  if (m_gameFileNameOffset)
  {
    savestateBuilder.add_game_file_name(*m_gameFileNameOffset);
    m_gameFileNameOffset.reset();
  }

  savestateBuilder.add_timestamp_frames(m_timestampFrames);

  const uint64_t wallClockNs = static_cast<uint64_t>(m_timestampWallClock * 1000.0 * 1000.0 * 1000.0);
  savestateBuilder.add_timestamp_wall_clock_ns(wallClockNs);

  if (m_emulatorAddonIdOffset)
  {
    savestateBuilder.add_emulator_addon_id(*m_emulatorAddonIdOffset);
    m_emulatorAddonIdOffset.reset();
  }

  if (m_emulatorVersionOffset)
  {
    savestateBuilder.add_emulator_version(*m_emulatorVersionOffset);
    m_emulatorVersionOffset.reset();
  }

  if (m_memoryDataOffset)
  {
    savestateBuilder.add_memory_data(*m_memoryDataOffset);
    m_memoryDataOffset.reset();
  }

  auto savestate = savestateBuilder.Finish();
  FinishSavestateBuffer(*m_builder, savestate);

  m_savestate = GetSavestate(m_builder->GetBufferPointer());
}

bool CSavestateFlatBuffer::Deserialize(std::vector<uint8_t> data)
{
  flatbuffers::Verifier verifier(data.data(), data.size());
  if (VerifySavestateBuffer(verifier))
  {
    const Savestate *savestate = GetSavestate(data.data());

    if (savestate->version() != SCHEMA_VERSION)
    {
      CLog::Log(LOGERROR, "RetroPlayer[SAVE): Schema version %u not supported, must be version %u",
        savestate->version(),
        SCHEMA_VERSION);
    }
    else
    {
      m_data = std::move(data);
      m_savestate = GetSavestate(m_data.data());
      return true;
    }
  }

  return false;
}
