/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClientFastForward.h"

#include "ServiceBroker.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/ThreadMessage.h"

#include <mutex>
#include <utility>

using namespace KODI;
using namespace GAME;

namespace
{
class CApplicationPlayerSpeed : public CGameClientFastForward::IPlayerSpeed
{
public:
  bool IsPlayingGame() const override
  {
    const auto player = Player();
    return player && player->IsPlayingGame();
  }

  float GetSpeed() const override
  {
    const auto player = Player();
    return player ? player->GetPlaySpeed() : 1.0f;
  }

  void SetSpeed(float speed) override
  {
    if (const auto player = Player(); player)
      player->SetPlaySpeed(speed);
  }

private:
  static std::shared_ptr<CApplicationPlayer> Player()
  {
    return CServiceBroker::GetAppComponents().GetComponent<CApplicationPlayer>();
  }
};

bool PostToApplication(std::function<void()> task)
{
  const auto messenger = CServiceBroker::GetAppMessenger();
  if (!messenger)
    return false;

  struct Callback
  {
    MESSAGING::ThreadMessageCallback message;
    std::function<void()> task;
  };
  auto* callback = new Callback{{}, std::move(task)};
  callback->message.userptr = callback;
  callback->message.callback = [](void* data)
  {
    std::unique_ptr<Callback> callback(static_cast<Callback*>(data));
    callback->task();
  };
  messenger->PostMsg(TMSG_CALLBACK, -1, -1, &callback->message);
  return true;
}

double RequestedSpeed(bool fastForward, double ratio)
{
  if (!fastForward)
    return 0.0;

  // Also false for a ratio that isn't a number
  if (ratio > 1.0 && ratio <= CGameClientFastForward::DEFAULT_SPEED)
    return ratio;

  return CGameClientFastForward::DEFAULT_SPEED;
}
} // namespace

struct CGameClientFastForward::State
{
  std::mutex mutex;

  double requested = 0.0; // The speed asked for last, or 0
  unsigned int session = 0; // Counts the games closed
  float owned = 0.0f; // The speed Kodi set and the player took, or 0

  // What the client reads back while requests wait for the application thread
  unsigned int pending = 0;
  double expectedSpeed = 0.0;
  double expectedOwned = 0.0;
};

CGameClientFastForward::CGameClientFastForward()
  : CGameClientFastForward(std::make_shared<CApplicationPlayerSpeed>(), PostToApplication)
{
}

CGameClientFastForward::CGameClientFastForward(std::shared_ptr<IPlayerSpeed> player, Post post)
  : m_player(std::move(player)),
    m_post(std::move(post)),
    m_state(std::make_shared<State>())
{
}

CGameClientFastForward::~CGameClientFastForward() = default;

void CGameClientFastForward::Request(bool fastForward, double ratio, double clientSpeed)
{
  unsigned int session = 0;
  {
    std::unique_lock lock(m_state->mutex);
    State& state = *m_state;

    state.requested = RequestedSpeed(fastForward, ratio);

    const bool expecting = state.pending > 0;
    const double speed = expecting ? state.expectedSpeed : clientSpeed;
    const double owned = expecting ? state.expectedOwned : static_cast<double>(state.owned);
    const double target = Target(state.requested, speed, owned);

    state.expectedOwned = state.requested > 0.0 ? (target != speed ? target : owned) : 0.0;
    state.expectedSpeed = target;
    ++state.pending;
    session = state.session;
  }

  // The task holds the state rather than this, which can go with its client first
  if (!m_post([state = m_state, player = m_player, session] { Apply(*state, *player, session); }))
  {
    std::unique_lock lock(m_state->mutex);
    if (m_state->session == session && m_state->pending > 0)
      --m_state->pending;
  }
}

double CGameClientFastForward::ClientSpeed(double clientSpeed) const
{
  std::unique_lock lock(m_state->mutex);
  return m_state->pending > 0 ? m_state->expectedSpeed : clientSpeed;
}

void CGameClientFastForward::Reset()
{
  std::unique_lock lock(m_state->mutex);
  m_state->requested = 0.0;
  m_state->owned = 0.0f;
  m_state->pending = 0;
  ++m_state->session;
}

double CGameClientFastForward::Target(double requested, double speed, double owned)
{
  if (requested == 0.0)
    return owned != 0.0 && speed == owned ? 1.0 : speed;

  // From normal speed, or from the speed Kodi set for an earlier ratio
  const bool kodiSpeed = owned != 0.0 ? speed == owned : speed == 1.0;
  return kodiSpeed ? requested : speed;
}

void CGameClientFastForward::Apply(State& state, IPlayerSpeed& player, unsigned int session)
{
  double requested = 0.0;
  float owned = 0.0f;
  {
    std::unique_lock lock(state.mutex);
    if (state.session != session)
      return;
    requested = state.requested;
    owned = state.owned;
  }

  // Not under the lock: changing speed waits on the game loop, whose frame may
  // be making a request
  bool changed = false;
  float taken = 0.0f;
  if (player.IsPlayingGame())
  {
    const float speed = player.GetSpeed();
    const auto target = static_cast<float>(
        Target(requested, static_cast<double>(speed), static_cast<double>(owned)));
    if (target != speed)
    {
      player.SetSpeed(target);

      // The player can round the speed or refuse it
      taken = player.GetSpeed();
      changed = taken != speed;
    }
  }

  std::unique_lock lock(state.mutex);
  if (state.session != session)
    return;

  if (requested == 0.0)
    state.owned = 0.0f;
  else if (changed)
    state.owned = taken;

  if (state.pending > 0)
    --state.pending;
}
