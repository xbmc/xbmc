/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ISavestate.h"

#include <memory>

#include <flatbuffers/flatbuffers.h>

namespace flatbuffers
{
class FlatBufferBuilder;
}

namespace KODI
{
namespace RETRO
{
struct Savestate;
struct SavestateBuilder;

class CSavestateFlatBuffer : public ISavestate
{
public:
  CSavestateFlatBuffer();
  ~CSavestateFlatBuffer() override;

  // Implementation of ISavestate
  void Reset() override;
  bool Serialize(const uint8_t*& data, size_t& size) const override;
  SAVE_TYPE Type() const override;
  uint8_t Slot() const override;
  std::string Label() const override;
  CDateTime Created() const override;
  std::string GameFileName() const override;
  uint64_t TimestampFrames() const override;
  double TimestampWallClock() const override;
  std::string GameClientID() const override;
  std::string GameClientVersion() const override;
  const uint8_t* GetMemoryData() const override;
  size_t GetMemorySize() const override;
  void SetType(SAVE_TYPE type) override;
  void SetSlot(uint8_t slot) override;
  void SetLabel(const std::string& label) override;
  void SetCreated(const CDateTime& created) override;
  void SetGameFileName(const std::string& gameFileName) override;
  void SetTimestampFrames(uint64_t timestampFrames) override;
  void SetTimestampWallClock(double timestampWallClock) override;
  void SetGameClientID(const std::string& gameClient) override;
  void SetGameClientVersion(const std::string& gameClient) override;
  uint8_t* GetMemoryBuffer(size_t size) override;
  void Finalize() override;
  bool Deserialize(std::vector<uint8_t> data) override;

private:
  /*!
   * \brief Helper class to hold data needed in creation of a FlatBuffer
   *
   * The builder is used when deserializing from individual fields.
   */
  std::unique_ptr<flatbuffers::FlatBufferBuilder> m_builder;

  /*!
   * \brief System memory storage (for deserializing savestates)
   *
   * This memory is used when deserializing from a vector.
   */
  std::vector<uint8_t> m_data;

  /*!
   * \brief FlatBuffer struct used for accessing data
   */
  const Savestate* m_savestate = nullptr;

  using StringOffset = flatbuffers::Offset<flatbuffers::String>;
  using VectorOffset = flatbuffers::Offset<flatbuffers::Vector<uint8_t>>;

  // Temporary deserialization variables
  SAVE_TYPE m_type = SAVE_TYPE::UNKNOWN;
  uint8_t m_slot = 0;
  std::unique_ptr<StringOffset> m_labelOffset;
  std::unique_ptr<StringOffset> m_createdOffset;
  std::unique_ptr<StringOffset> m_gameFileNameOffset;
  uint64_t m_timestampFrames = 0;
  double m_timestampWallClock = 0.0;
  std::unique_ptr<StringOffset> m_emulatorAddonIdOffset;
  std::unique_ptr<StringOffset> m_emulatorVersionOffset;
  std::unique_ptr<VectorOffset> m_memoryDataOffset;
};
} // namespace RETRO
} // namespace KODI
