/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <functional>
#include <memory>

namespace KODI
{
namespace GAME
{
/*!
 * \ingroup games
 *
 * \brief A game client's requests to fast-forward
 *
 * A client asks from inside a frame, but the player changes speed on the
 * application thread, as it does when the user picks a speed, so it can't race
 * the player closing. Requests are applied there in the order they're made,
 * each one applying the latest.
 *
 * Kodi only fast-forwards from normal speed, and only returns to normal speed
 * from the speed it set. Any other speed is the user's and is left alone.
 */
class CGameClientFastForward
{
public:
  //! The speed a client gets when it leaves the speed to Kodi, and the most it
  //! can ask for. The game loop goes no faster than the client runs frames.
  static constexpr double DEFAULT_SPEED = 16.0;

  //! The player whose speed the requests change
  class IPlayerSpeed
  {
  public:
    virtual ~IPlayerSpeed() = default;

    virtual bool IsPlayingGame() const = 0;
    virtual float GetSpeed() const = 0;
    virtual void SetSpeed(float speed) = 0;
  };

  //! Runs a task on the application thread later, or returns false
  using Post = std::function<bool(std::function<void()> task)>;

  //! Requests that change the speed of the application's player
  CGameClientFastForward();

  CGameClientFastForward(std::shared_ptr<IPlayerSpeed> player, Post post);

  ~CGameClientFastForward();

  /*!
   * \brief Ask to fast-forward, or to stop a fast-forward asked for
   *
   * \param fastForward True to fast-forward, false to return to normal speed
   * \param ratio The speed asked for if it's above 1 and at most
   *        DEFAULT_SPEED, otherwise DEFAULT_SPEED
   * \param clientSpeed The speed the client was last told it's playing at
   */
  void Request(bool fastForward, double ratio, double clientSpeed);

  /*!
   * \brief The speed the client reads back
   *
   * Until its requests are applied, the speed they will give it, as it would
   * get at once from RetroArch. A client may act on the speed it reads in its
   * next frame.
   *
   * \param clientSpeed The speed the client was last told it's playing at
   */
  double ClientSpeed(double clientSpeed) const;

  /*!
   * \brief The game closed: drop requests not yet applied, and forget the
   *        speed Kodi set
   */
  void Reset();

  /*!
   * \brief The speed a request moves the player to
   *
   * \param requested The speed asked for, or 0 to stop fast-forwarding
   * \param speed The player's speed
   * \param owned The speed Kodi set for the client, or 0 if it set none
   */
  static double Target(double requested, double speed, double owned);

private:
  struct State;

  static void Apply(State& state, IPlayerSpeed& player, unsigned int session);

  const std::shared_ptr<IPlayerSpeed> m_player;
  const Post m_post;
  const std::shared_ptr<State> m_state;
};
} // namespace GAME
} // namespace KODI
