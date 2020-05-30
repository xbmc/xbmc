/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ReversiblePlayback.h"

#include "ServiceBroker.h"
#include "cores/RetroPlayer/savestates/ISavestate.h"
#include "cores/RetroPlayer/savestates/SavestateDatabase.h"
#include "cores/RetroPlayer/streams/memory/DeltaPairMemoryStream.h"
#include "games/GameServices.h"
#include "games/GameSettings.h"
#include "games/addons/GameClient.h"
#include "threads/SingleLock.h"
#include "utils/MathUtils.h"
#include "utils/URIUtils.h"

#include <algorithm>

using namespace KODI;
using namespace RETRO;

#define REWIND_FACTOR 0.25 // Rewind at 25% of gameplay speed

CReversiblePlayback::CReversiblePlayback(GAME::CGameClient* gameClient,
                                         double fps,
                                         size_t serializeSize)
  : m_gameClient(gameClient),
    m_gameLoop(this, fps),
    m_savestateDatabase(new CSavestateDatabase),
    m_totalFrameCount(0),
    m_pastFrameCount(0),
    m_futureFrameCount(0),
    m_playTimeMs(0),
    m_totalTimeMs(0),
    m_cacheTimeMs(0)
{
  UpdateMemoryStream();

  GAME::CGameSettings& gameSettings = CServiceBroker::GetGameServices().GameSettings();
  gameSettings.RegisterObserver(this);
}

CReversiblePlayback::~CReversiblePlayback()
{
  GAME::CGameSettings& gameSettings = CServiceBroker::GetGameServices().GameSettings();
  gameSettings.UnregisterObserver(this);

  Deinitialize();
}

void CReversiblePlayback::Initialize()
{
  m_gameLoop.Start();
}

void CReversiblePlayback::Deinitialize()
{
  m_gameLoop.Stop();
}

void CReversiblePlayback::SeekTimeMs(unsigned int timeMs)
{
  const int offsetTimeMs = timeMs - GetTimeMs();
  const int offsetFrames = MathUtils::round_int(offsetTimeMs / 1000.0 * m_gameLoop.FPS());

  if (offsetFrames > 0)
  {
    const uint64_t frames = std::min(static_cast<uint64_t>(offsetFrames), m_futureFrameCount);
    if (frames > 0)
    {
      m_gameLoop.SetSpeed(0.0);
      AdvanceFrames(frames);
      m_gameLoop.SetSpeed(1.0);
    }
  }
  else if (offsetFrames < 0)
  {
    const uint64_t frames = std::min(static_cast<uint64_t>(-offsetFrames), m_pastFrameCount);
    if (frames > 0)
    {
      m_gameLoop.SetSpeed(0.0);
      RewindFrames(frames);
      m_gameLoop.SetSpeed(1.0);
    }
  }
}

double CReversiblePlayback::GetSpeed() const
{
  return m_gameLoop.GetSpeed();
}

void CReversiblePlayback::SetSpeed(double speedFactor)
{
  if (speedFactor >= 0.0)
    m_gameLoop.SetSpeed(speedFactor);
  else
    m_gameLoop.SetSpeed(speedFactor * REWIND_FACTOR);
}

void CReversiblePlayback::PauseAsync()
{
  m_gameLoop.PauseAsync();
}

std::string CReversiblePlayback::CreateSavestate()
{
  const size_t memorySize = m_gameClient->SerializeSize();

  // Game client must support serialization
  if (memorySize == 0)
    return "";

  //! @todo Handle savestates for standalone game clients
  if (m_gameClient->GetGamePath().empty())
  {
    return "";
  }

  const CDateTime now = CDateTime::GetCurrentDateTime();
  const std::string label = now.GetAsLocalizedDateTime();
  const std::string gameFileName = URIUtils::GetFileName(m_gameClient->GetGamePath());
  const uint64_t timestampFrames = m_totalFrameCount;
  const double timestampWallClock =
      (m_totalFrameCount /
       m_gameClient->GetFrameRate()); //! @todo Accumulate playtime instead of deriving it
  const std::string gameClientId = m_gameClient->ID();
  const std::string gameClientVersion = m_gameClient->Version().asString();

  std::unique_ptr<ISavestate> savestate = m_savestateDatabase->CreateSavestate();

  savestate->SetType(SAVE_TYPE::AUTO);
  savestate->SetLabel(label);
  savestate->SetCreated(now);
  savestate->SetGameFileName(gameFileName);
  savestate->SetTimestampFrames(timestampFrames);
  savestate->SetTimestampWallClock(timestampWallClock);
  savestate->SetGameClientID(gameClientId);
  savestate->SetGameClientVersion(gameClientVersion);

  uint8_t* memoryData = savestate->GetMemoryBuffer(memorySize);

  {
    CSingleLock lock(m_mutex);
    if (m_memoryStream && m_memoryStream->CurrentFrame() != nullptr)
    {
      std::memcpy(memoryData, m_memoryStream->CurrentFrame(), memorySize);
    }
    else
    {
      lock.Leave();
      if (!m_gameClient->Serialize(memoryData, memorySize))
        return "";
    }
  }

  savestate->Finalize();

  if (!m_savestateDatabase->AddSavestate(m_gameClient->GetGamePath(), *savestate))
    return "";

  return m_gameClient->GetGamePath();
}

bool CReversiblePlayback::LoadSavestate(const std::string& path)
{
  const size_t memorySize = m_gameClient->SerializeSize();

  // Game client must support serialization
  if (memorySize == 0)
    return false;

  bool bSuccess = false;

  std::unique_ptr<ISavestate> savestate = m_savestateDatabase->CreateSavestate();
  if (m_savestateDatabase->GetSavestate(path, *savestate) &&
      savestate->GetMemorySize() == memorySize)
  {
    {
      CSingleLock lock(m_mutex);
      if (m_memoryStream)
      {
        m_memoryStream->SetFrameCounter(savestate->TimestampFrames());
        std::memcpy(m_memoryStream->BeginFrame(), savestate->GetMemoryData(), memorySize);
        m_memoryStream->SubmitFrame();
      }
    }

    if (m_gameClient->Deserialize(savestate->GetMemoryData(), memorySize))
    {
      m_totalFrameCount = savestate->TimestampFrames();
      bSuccess = true;
    }
  }

  return bSuccess;
}

void CReversiblePlayback::FrameEvent()
{
  m_gameClient->RunFrame();

  AddFrame();
}

void CReversiblePlayback::RewindEvent()
{
  RewindFrames(1);

  m_gameClient->RunFrame();
}

void CReversiblePlayback::AddFrame()
{
  CSingleLock lock(m_mutex);

  if (m_memoryStream)
  {
    if (m_gameClient->Serialize(m_memoryStream->BeginFrame(), m_memoryStream->FrameSize()))
    {
      m_memoryStream->SubmitFrame();
      UpdatePlaybackStats();
    }
  }

  m_totalFrameCount++;
}

void CReversiblePlayback::RewindFrames(uint64_t frames)
{
  CSingleLock lock(m_mutex);

  if (m_memoryStream)
  {
    m_memoryStream->RewindFrames(frames);
    m_gameClient->Deserialize(m_memoryStream->CurrentFrame(), m_memoryStream->FrameSize());
    UpdatePlaybackStats();
  }

  m_totalFrameCount -= std::min(m_totalFrameCount, frames);
}

void CReversiblePlayback::AdvanceFrames(uint64_t frames)
{
  CSingleLock lock(m_mutex);

  if (m_memoryStream)
  {
    m_memoryStream->AdvanceFrames(frames);
    m_gameClient->Deserialize(m_memoryStream->CurrentFrame(), m_memoryStream->FrameSize());
    UpdatePlaybackStats();
  }

  m_totalFrameCount += frames;
}

void CReversiblePlayback::UpdatePlaybackStats()
{
  m_pastFrameCount = m_memoryStream->PastFramesAvailable();
  m_futureFrameCount = m_memoryStream->FutureFramesAvailable();

  const uint64_t played = m_pastFrameCount + (m_memoryStream->CurrentFrame() ? 1 : 0);
  const uint64_t total = m_memoryStream->MaxFrameCount();
  const uint64_t cached = m_futureFrameCount;

  m_playTimeMs = MathUtils::round_int(1000.0 * played / m_gameLoop.FPS());
  m_totalTimeMs = MathUtils::round_int(1000.0 * total / m_gameLoop.FPS());
  m_cacheTimeMs = MathUtils::round_int(1000.0 * cached / m_gameLoop.FPS());
}

void CReversiblePlayback::Notify(const Observable& obs, const ObservableMessage msg)
{
  switch (msg)
  {
    case ObservableMessageSettingsChanged:
      UpdateMemoryStream();
      break;
    default:
      break;
  }
}

void CReversiblePlayback::UpdateMemoryStream()
{
  CSingleLock lock(m_mutex);

  bool bRewindEnabled = false;

  GAME::CGameSettings& gameSettings = CServiceBroker::GetGameServices().GameSettings();

  if (m_gameClient->SerializeSize() > 0)
    bRewindEnabled = gameSettings.RewindEnabled();

  if (bRewindEnabled)
  {
    unsigned int rewindBufferSec = gameSettings.MaxRewindTimeSec();
    if (rewindBufferSec < 10)
      rewindBufferSec = 10; // Sanity check

    unsigned int frameCount = MathUtils::round_int(rewindBufferSec * m_gameLoop.FPS());

    if (!m_memoryStream)
    {
      m_memoryStream.reset(new CDeltaPairMemoryStream);
      m_memoryStream->Init(m_gameClient->SerializeSize(), frameCount);
    }

    if (m_memoryStream->MaxFrameCount() != frameCount)
    {
      m_memoryStream->SetMaxFrameCount(frameCount);
    }
  }
  else
  {
    m_memoryStream.reset();

    // Reset playback stats
    m_pastFrameCount = 0;
    m_futureFrameCount = 0;
    m_playTimeMs = 0;
    m_totalTimeMs = 0;
    m_cacheTimeMs = 0;
  }
}
