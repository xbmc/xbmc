/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "application/ApplicationPlayLists.h"
#include "application/IApplicationComponent.h"
#include "guilib/IMsgTargetCallback.h"
#include "interfaces/AnnouncementEvents.h"

#include <functional>
#include <memory>
#include <optional>
#include <string>

class CFileItem;
class CVariant;

/*!
 * \brief The one publisher of Player and Playlist notifications. The playlists report what the
 * entry claims and what changed on them, the player what it renders, and the slideshow what it
 * shows.
 */
class CPlaybackAnnouncer : public IApplicationComponent,
                           public IMsgTargetCallback,
                           public CApplicationPlayLists::IObserver
{
public:
  /*!
   * \brief A transition of the picture slideshow.
   */
  enum class SlideShowEvent
  {
    Play,
    Pause,
    Stop
  };

  using Sink = std::function<void(const ANNOUNCEMENT::Announcement& announcement)>;

  /*!
   * \param sink Where notifications go; the announcement manager when none is given.
   */
  explicit CPlaybackAnnouncer(std::shared_ptr<CApplicationPlayLists> playLists, Sink sink = {});
  ~CPlaybackAnnouncer() override;

  bool OnMessage(CGUIMessage& message) override;

  /*!
   * \param item The entry being described, or nullptr for what plays.
   * \param claimed Answer what the entry claims rather than what the player renders, as when it
   * starts: the playlist it is on and whether Audio follows it; a channel by whether it is radio.
   * Otherwise the player answers, once it renders anything.
   * \return The kinds of stream the playback carries.
   */
  KODI::MEDIA::Streams GetStreams(const CFileItem* item, bool claimed) const;

  /*!
   * \brief Publish a change to properties of what is playing, with the streams it carries.
   * Nothing is published while nothing plays.
   */
  void Announce(ANNOUNCEMENT::EVENT::PLAYER::PropertiesChanged changed) const;

  //! Publish Player.OnContentGeometryChange, carrying the players as every Player notification does.
  void OnContentGeometryChanged(CVariant data) const;

  /*!
   * \brief The picture slideshow reports what it does.
   * \param slide The slide it shows.
   * \param running Whether the slides advance by themselves, for Play.
   */
  void OnSlideShow(SlideShowEvent event,
                   const std::shared_ptr<const CFileItem>& slide,
                   bool running);
  void OnSlideShowShuffled() const;
  void OnSlideShowListChanged(const KODI::PLAYLIST::PlayListChange& change) const;

  void OnStarted(const std::shared_ptr<CFileItem>& started) override;
  void OnListChanged(KODI::PLAYLIST::Type type,
                     const KODI::PLAYLIST::PlayListChange& change) override;
  void OnShuffled(KODI::PLAYLIST::Type type, bool shuffled) override;
  void OnRepeat(KODI::PLAYLIST::Type type, KODI::PLAYLIST::Repeat repeat) override;
  void OnFeed(bool playing) override;
  void OnFailed(const std::shared_ptr<const CFileItem>& item,
                KODI::PLAYLIST::FailReason reason) override;

private:
  void PublishListChange(std::optional<KODI::PLAYLIST::Type> playList,
                         const KODI::PLAYLIST::PlayListChange& change) const;

  std::shared_ptr<CApplicationPlayLists> m_playLists;
  Sink m_sink;
};
