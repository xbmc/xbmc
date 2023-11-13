/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SavestateFlatBuffer.h"

#include "XBDateTime.h"
#include "savestate_generated.h"
#include "utils/log.h"
#include "video_generated.h"

#include <memory>

using namespace KODI;
using namespace RETRO;

namespace
{
const uint8_t SCHEMA_VERSION = 3;
const uint8_t SCHEMA_MIN_VERSION = 1;

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

/*!
 * \brief Translate the video pixel format (RetroPlayer to FlatBuffers)
 */
PixelFormat TranslatePixelFormat(AVPixelFormat pixelFormat)
{
  switch (pixelFormat)
  {
    case AV_PIX_FMT_RGBA:
      return PixelFormat_RGBA_8888;

    case AV_PIX_FMT_0RGB32:
#if defined(__BIG_ENDIAN__)
      return PixelFormat_XRGB_8888;
#else
      return PixelFormat_BGRX_8888;
#endif

    case AV_PIX_FMT_RGB565:
#if defined(__BIG_ENDIAN__)
      return PixelFormat_RGB_565_BE;
#else
      return PixelFormat_RGB_565_LE;
#endif

    case AV_PIX_FMT_RGB555:
#if defined(__BIG_ENDIAN__)
      return PixelFormat_RGB_555_BE;
#else
      return PixelFormat_RGB_555_LE;
#endif

    default:
      break;
  }

  return PixelFormat_Unknown;
}

/*!
 * \brief Translate the video pixel format (FlatBuffers to RetroPlayer)
 */
AVPixelFormat TranslatePixelFormat(PixelFormat pixelFormat)
{
  switch (pixelFormat)
  {
    case PixelFormat_RGBA_8888:
      return AV_PIX_FMT_RGBA;

    case PixelFormat_XRGB_8888:
      return AV_PIX_FMT_0RGB;

    case PixelFormat_BGRX_8888:
      return AV_PIX_FMT_BGR0;

    case PixelFormat_RGB_565_BE:
      return AV_PIX_FMT_RGB565BE;

    case PixelFormat_RGB_565_LE:
      return AV_PIX_FMT_RGB565LE;

    case PixelFormat_RGB_555_BE:
      return AV_PIX_FMT_RGB555BE;

    case PixelFormat_RGB_555_LE:
      return AV_PIX_FMT_RGB555LE;

    default:
      break;
  }

  return AV_PIX_FMT_NONE;
}

/*!
 * \brief Translate the video rotation (RetroPlayer to FlatBuffers)
 */
VideoRotation TranslateRotation(unsigned int rotationCCW)
{
  switch (rotationCCW)
  {
    case 0:
      return VideoRotation_CCW_0;
    case 90:
      return VideoRotation_CCW_90;
    case 180:
      return VideoRotation_CCW_180;
    case 270:
      return VideoRotation_CCW_270;
    default:
      break;
  }

  return VideoRotation_CCW_0;
}

/*!
 * \brief Translate the video rotation (RetroPlayer to FlatBuffers)
 */
unsigned int TranslateRotation(VideoRotation rotationCCW)
{
  switch (rotationCCW)
  {
    case VideoRotation_CCW_0:
      return 0;
    case VideoRotation_CCW_90:
      return 90;
    case VideoRotation_CCW_180:
      return 180;
    case VideoRotation_CCW_270:
      return 270;
    default:
      break;
  }

  return 0;
}
} // namespace

CSavestateFlatBuffer::CSavestateFlatBuffer()
{
  Reset();
}

CSavestateFlatBuffer::~CSavestateFlatBuffer() = default;

void CSavestateFlatBuffer::Reset()
{
  m_builder = std::make_unique<flatbuffers::FlatBufferBuilder>(INITIAL_FLATBUFFER_SIZE);
  m_data.clear();
  m_savestate = nullptr;
}

bool CSavestateFlatBuffer::Serialize(const uint8_t*& data, size_t& size) const
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

void CSavestateFlatBuffer::SetLabel(const std::string& label)
{
  m_labelOffset = std::make_unique<StringOffset>(m_builder->CreateString(label));
}

std::string CSavestateFlatBuffer::Caption() const
{
  std::string caption;

  if (m_savestate != nullptr && m_savestate->caption())
    caption = m_savestate->caption()->str();

  return caption;
}

void CSavestateFlatBuffer::SetCaption(const std::string& caption)
{
  m_captionOffset = std::make_unique<StringOffset>(m_builder->CreateString(caption));
}

CDateTime CSavestateFlatBuffer::Created() const
{
  CDateTime createdUTC;

  if (m_savestate != nullptr && m_savestate->created())
    createdUTC.SetFromW3CDateTime(m_savestate->created()->c_str(), false);

  return createdUTC;
}

void CSavestateFlatBuffer::SetCreated(const CDateTime& createdUTC)
{
  m_createdOffset =
      std::make_unique<StringOffset>(m_builder->CreateString(createdUTC.GetAsW3CDateTime(true)));
}

std::string CSavestateFlatBuffer::GameFileName() const
{
  std::string gameFileName;

  if (m_savestate != nullptr && m_savestate->game_file_name())
    gameFileName = m_savestate->game_file_name()->c_str();

  return gameFileName;
}

void CSavestateFlatBuffer::SetGameFileName(const std::string& gameFileName)
{
  m_gameFileNameOffset = std::make_unique<StringOffset>(m_builder->CreateString(gameFileName));
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

void CSavestateFlatBuffer::SetGameClientID(const std::string& gameClientId)
{
  m_emulatorAddonIdOffset = std::make_unique<StringOffset>(m_builder->CreateString(gameClientId));
}

std::string CSavestateFlatBuffer::GameClientVersion() const
{
  std::string gameClientVersion;

  if (m_savestate != nullptr && m_savestate->emulator_version())
    gameClientVersion = m_savestate->emulator_version()->c_str();

  return gameClientVersion;
}

void CSavestateFlatBuffer::SetGameClientVersion(const std::string& gameClientVersion)
{
  m_emulatorVersionOffset =
      std::make_unique<StringOffset>(m_builder->CreateString(gameClientVersion));
}

AVPixelFormat CSavestateFlatBuffer::GetPixelFormat() const
{
  if (m_savestate != nullptr)
    return TranslatePixelFormat(m_savestate->pixel_format());

  return AV_PIX_FMT_NONE;
}

void CSavestateFlatBuffer::SetPixelFormat(AVPixelFormat pixelFormat)
{
  m_pixelFormat = pixelFormat;
}

unsigned int CSavestateFlatBuffer::GetNominalWidth() const
{
  if (m_savestate != nullptr)
    return m_savestate->nominal_width();

  return 0;
}

void CSavestateFlatBuffer::SetNominalWidth(unsigned int nominalWidth)
{
  m_nominalWidth = nominalWidth;
}

unsigned int CSavestateFlatBuffer::GetNominalHeight() const
{
  if (m_savestate != nullptr)
    return m_savestate->nominal_height();

  return 0;
}

void CSavestateFlatBuffer::SetNominalHeight(unsigned int nominalHeight)
{
  m_nominalHeight = nominalHeight;
}

unsigned int CSavestateFlatBuffer::GetMaxWidth() const
{
  if (m_savestate != nullptr)
    return m_savestate->max_width();

  return 0;
}

void CSavestateFlatBuffer::SetMaxWidth(unsigned int maxWidth)
{
  m_maxWidth = maxWidth;
}

unsigned int CSavestateFlatBuffer::GetMaxHeight() const
{
  if (m_savestate != nullptr)
    return m_savestate->max_height();

  return 0;
}

void CSavestateFlatBuffer::SetMaxHeight(unsigned int maxHeight)
{
  m_maxHeight = maxHeight;
}

float CSavestateFlatBuffer::GetPixelAspectRatio() const
{
  if (m_savestate != nullptr)
    return m_savestate->pixel_aspect_ratio();

  return 0.0f;
}

void CSavestateFlatBuffer::SetPixelAspectRatio(float pixelAspectRatio)
{
  m_pixelAspectRatio = pixelAspectRatio;
}

const uint8_t* CSavestateFlatBuffer::GetVideoData() const
{
  if (m_savestate != nullptr && m_savestate->video_data())
    return m_savestate->video_data()->data();

  return nullptr;
}

size_t CSavestateFlatBuffer::GetVideoSize() const
{
  if (m_savestate != nullptr && m_savestate->video_data())
    return m_savestate->video_data()->size();

  return 0;
}

uint8_t* CSavestateFlatBuffer::GetVideoBuffer(size_t size)
{
  uint8_t* videoBuffer = nullptr;

  m_videoDataOffset =
      std::make_unique<VectorOffset>(m_builder->CreateUninitializedVector(size, &videoBuffer));

  return videoBuffer;
}

unsigned int CSavestateFlatBuffer::GetVideoWidth() const
{
  if (m_savestate != nullptr)
    return m_savestate->video_width();

  return 0;
}

void CSavestateFlatBuffer::SetVideoWidth(unsigned int videoWidth)
{
  m_videoWidth = videoWidth;
}

unsigned int CSavestateFlatBuffer::GetVideoHeight() const
{
  if (m_savestate != nullptr)
    return m_savestate->video_height();

  return 0;
}

void CSavestateFlatBuffer::SetVideoHeight(unsigned int videoHeight)
{
  m_videoHeight = videoHeight;
}

unsigned int CSavestateFlatBuffer::GetRotationDegCCW() const
{
  if (m_savestate != nullptr)
    return TranslateRotation(m_savestate->rotation_ccw());

  return 0;
}

void CSavestateFlatBuffer::SetRotationDegCCW(unsigned int rotationCCW)
{
  m_rotationCCW = rotationCCW;
}

const uint8_t* CSavestateFlatBuffer::GetMemoryData() const
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

uint8_t* CSavestateFlatBuffer::GetMemoryBuffer(size_t size)
{
  uint8_t* memoryBuffer = nullptr;

  m_memoryDataOffset =
      std::make_unique<VectorOffset>(m_builder->CreateUninitializedVector(size, &memoryBuffer));

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

  if (m_captionOffset)
  {
    savestateBuilder.add_caption(*m_captionOffset);
    m_captionOffset.reset();
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

  const uint64_t wallClockNs =
      static_cast<uint64_t>(m_timestampWallClock * 1000.0 * 1000.0 * 1000.0);
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

  savestateBuilder.add_pixel_format(TranslatePixelFormat(m_pixelFormat));

  savestateBuilder.add_nominal_width(m_nominalWidth);

  savestateBuilder.add_nominal_height(m_nominalHeight);

  savestateBuilder.add_max_width(m_maxWidth);

  savestateBuilder.add_max_height(m_maxHeight);

  savestateBuilder.add_pixel_aspect_ratio(m_pixelAspectRatio);

  if (m_videoDataOffset)
  {
    savestateBuilder.add_video_data(*m_videoDataOffset);
    m_videoDataOffset.reset();
  }

  savestateBuilder.add_video_width(m_videoWidth);

  savestateBuilder.add_video_height(m_videoHeight);

  savestateBuilder.add_rotation_ccw(TranslateRotation(m_rotationCCW));

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
    const Savestate* savestate = GetSavestate(data.data());

    if (savestate->version() < SCHEMA_MIN_VERSION)
    {
      CLog::Log(LOGERROR,
                "RetroPlayer[SAVE): Schema version {} not supported, must be at least version {}",
                savestate->version(), SCHEMA_MIN_VERSION);
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
