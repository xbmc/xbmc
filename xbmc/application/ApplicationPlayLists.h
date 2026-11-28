/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "application/IApplicationComponent.h"
#include "application/PlaybackOptions.h"
#include "guilib/IMsgTargetCallback.h"
#include "media/MediaStreams.h"
#include "playlists/PlayListTypes.h"
#include "threads/CriticalSection.h"

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

class CFileItem;
class CFileItemList;

namespace KODI::PLAYLIST
{
class CPlayList;
class IEntryRules;
class IFeed;
struct PlayListChange;
} // namespace KODI::PLAYLIST

/*!
 * \brief The Video and Audio playlists, and what each is doing.
 *
 * Both playlists always exist, possibly empty. The player works through one of them at a time; a
 * play on the one it is working through, even of new contents, leaves the screen as it is.
 * While the Video playlist plays an entry that also holds audio, Audio follows Video: what is asked
 * for on Audio is queued on Video (see GetQueueType()).
 */
class CApplicationPlayLists : public IApplicationComponent, public IMsgTargetCallback
{
public:
  //! What a request to move on or back did.
  enum class Step
  {
    Played,
    //! Nothing was there to move to.
    NothingThere,
    Failed
  };

  enum class Persist
  {
    No,
    Yes
  };

  /*!
   * \brief Told what the player started and what changes on the playlists.
   */
  class IObserver
  {
  public:
    virtual ~IObserver() = default;
    //! What the player started, or nullptr if it reported no item.
    virtual void OnStarted(const std::shared_ptr<CFileItem>& started) = 0;
    virtual void OnListChanged(KODI::PLAYLIST::Type type,
                               const KODI::PLAYLIST::PlayListChange& change) = 0;
    virtual void OnShuffled(KODI::PLAYLIST::Type type, bool shuffled) = 0;
    virtual void OnRepeat(KODI::PLAYLIST::Type type, KODI::PLAYLIST::Repeat repeat) = 0;
    //! Whether a feed is playing changed.
    virtual void OnFeed(bool playing) = 0;
    virtual void OnFailed(const std::shared_ptr<const CFileItem>& item,
                          KODI::PLAYLIST::FailReason reason) = 0;
  };

  /*!
   * \brief Told what the playlists do by themselves, for the GUI to show.
   */
  class IGUIListener
  {
  public:
    virtual ~IGUIListener() = default;
    //! A playlist, or which one plays, changed.
    virtual void OnPlayListsChanged() = 0;
    //! A run ended.
    virtual void OnStopped() = 0;
    //! Playback was given up because too many entries in a row failed to play.
    virtual void OnEntriesFailed() = 0;
  };

  class IPlayback;

  explicit CApplicationPlayLists(std::unique_ptr<IPlayback> playback);
  ~CApplicationPlayLists() override;

  bool OnMessage(CGUIMessage& message) override;

  //! Report a play that was asked for and did not happen, where it was refused before reaching
  //! the playlists.
  void ReportFailed(const std::shared_ptr<const CFileItem>& item,
                    KODI::PLAYLIST::FailReason reason) const;

  /*!
   * \brief Go back an entry, unless the playing file is past its first few seconds and can seek,
   * a channel is playing, no playlist is playing, or the playlist is one entry that does not
   * repeat.
   * \return What going back did, or none if it was left to the player.
   */
  std::optional<Step> Previous();

  /*!
   * \brief Go on an entry, unless a channel is playing, no playlist is playing, or the playlist is
   * one entry that does not repeat.
   * \return What going on did, or none if it was left to the player.
   */
  std::optional<Step> Next();

  /*!
   * \brief A playlist to read. Every change goes through this component, which keeps the rules
   * for the entry being played.
   */
  const KODI::PLAYLIST::CPlayList& GetPlayList(KODI::PLAYLIST::Type type) const;

  /*!
   * \return The playlist the player is working through, if any.
   */
  std::optional<KODI::PLAYLIST::Type> GetPlayingType() const;

  /*!
   * \brief The playlist to show: the playing one, else the preferred one if it has entries, else
   * one that has entries, Video first.
   */
  std::optional<KODI::PLAYLIST::Type> GetTypeToShow(
      std::optional<KODI::PLAYLIST::Type> preferred) const;

  /*!
   * \brief Choose the playlist the player works through. Choosing another one drops the feed.
   */
  void SetPlayingType(KODI::PLAYLIST::Type type);

  /*!
   * \return Whether what plays is audio: the playing entry holds audio only or, for playback
   * started outside the playlists, the player is playing audio only.
   */
  bool IsPlayingAsAudio() const;

  //! What the playing playlist's current entry holds, or nothing when no playlist is playing.
  std::optional<KODI::MEDIA::Streams> GetPlayingStreams() const;

  /*!
   * \return Whether an item starting now plays as audio: what the playing entry holds or, when
   * no playlist is playing, what PLAYLIST::TypeFor() says of it.
   */
  bool IsStartingAsAudio(const CFileItem& item) const;

  /*!
   * \return What an entry holds, as recorded when it was placed, or what its playlist claims when
   * the item did not say.
   */
  KODI::MEDIA::Streams GetStreams(KODI::PLAYLIST::Type type, KODI::PLAYLIST::EntryId entry) const;

  /*!
   * \return While the player is working through this playlist, the position in list order of the
   * entry the given number of entries ahead of the current one in play order, or behind it for a
   * negative offset; otherwise, or if there is no such entry, -1.
   */
  int GetPlayingPosition(KODI::PLAYLIST::Type type, int offset = 0) const;

  /*!
   * \return While the player is working through this playlist, the current entry's place in play
   * order, for showing; otherwise -1.
   */
  int GetPlayingDisplayPosition(KODI::PLAYLIST::Type type) const;

  /*!
   * \return This playlist's current entry while the player is working through it, else NO_ENTRY.
   */
  KODI::PLAYLIST::EntryId GetPlayingEntry(KODI::PLAYLIST::Type type) const;

  //! While the player works through this playlist, whether an entry plays after the current one.
  bool HasNext(KODI::PLAYLIST::Type type) const;
  //! While the player works through this playlist, whether an entry plays before the current one.
  bool HasPrevious(KODI::PLAYLIST::Type type) const;
  //! While the player works through this playlist, whether an entry is this many from the current.
  bool HasEntry(KODI::PLAYLIST::Type type, int offset) const;

  /*!
   * \return Where items asked for on a playlist are queued: that playlist, except that while Audio
   * follows a film on Video, what is asked for on Audio goes on Video after it.
   */
  KODI::PLAYLIST::Type GetQueueType(KODI::PLAYLIST::Type requested) const;

  bool IsAudioFollowingVideo() const;

  // Declared outside the class: a nested aggregate's default member initializers cannot be used
  // by this class's own "= {}" default arguments.
  using OnFail = KODI::APPLICATION::OnFail;
  using PlayOptions = KODI::APPLICATION::PlayOptions;

  /*!
   * \brief The player, as the playlists use it; tests stand in for the application here.
   */
  class IPlayback
  {
  public:
    enum class Queued
    {
      Yes,
      //! The player would not take it.
      Refused,
      //! It could not be made ready for the player.
      Unresolved
    };

    virtual ~IPlayback() = default;
    /*!
     * \brief Open a file.
     * \return false if it failed to play; a cancelled choice is not a failure.
     */
    virtual bool Open(const CFileItem& item,
                      const PlayOptions& options,
                      KODI::APPLICATION::StartsRun startsRun) = 0;
    //! Fill in a library item's tag. \return false if there was nothing to fill in.
    virtual bool LoadLibraryTag(CFileItem& item) const = 0;
    //! Offer the player the file to start by itself once the current one ends.
    virtual Queued QueueNext(const CFileItem& item) = 0;
    //! Tell the player nothing follows for it to queue.
    virtual void NothingToQueue() = 0;
    virtual void Stop() = 0;
    virtual void Close() = 0;
    virtual bool IsPlaying() const = 0;
    virtual bool IsPlayingVideo() const = 0;
    virtual bool IsPlayingAudio() const = 0;
    //! The player in use, to open what follows in.
    virtual std::string GetName() const = 0;
    //! Whether previous restarts the playing file instead: it can seek and is past its start.
    virtual bool RestartsOnPrevious() const = 0;
  };

  /*!
   * \brief Play a playlist as it stands. While it is already playing this moves within the run,
   * and the play order is kept.
   * \param position A position in list order; on a new run its entry leads the play order. With
   * none, the playlist plays from the start of its play order.
   */
  bool PlayFrom(KODI::PLAYLIST::Type type,
                std::optional<int> position = std::nullopt,
                const PlayOptions& options = {});

  /*!
   * \brief Replace the playlist's contents with the items, and play it.
   * \param position As for PlayFrom(), in the items.
   * \param sourcePath The playlist file, smart playlist or folder the items were read from, if any;
   * see GetPlayingSourcePath().
   */
  bool PlayItems(KODI::PLAYLIST::Type type,
                 const CFileItemList& items,
                 std::optional<int> position = std::nullopt,
                 const PlayOptions& options = {},
                 const std::string& sourcePath = "");

  /*!
   * \brief Play items expanded from a selection, from the chosen one; a chosen item that did
   * not become one of them plays on its own.
   * \param start The chosen item's position in items, or -1 when it is not among them.
   * \param chosen The item chosen, or nullptr to play the items from the start.
   */
  bool PlayExpanded(KODI::PLAYLIST::Type type,
                    const CFileItemList& items,
                    int start,
                    const std::shared_ptr<CFileItem>& chosen,
                    const PlayOptions& options);

  /*!
   * \brief Play one item on the named playlist or, with none named, the one PLAYLIST::TypeFor()
   * gives it. It replaces the playlist's contents; on a fed playlist that is playing it plays next
   * instead, and the feed carries on.
   */
  bool PlayItem(std::optional<KODI::PLAYLIST::Type> named,
                const std::shared_ptr<CFileItem>& item,
                const PlayOptions& options = {});

  /*!
   * \return The playlist file, smart playlist or folder that the playing playlist was read from, or
   * empty once anything else has been added to it.
   */
  std::string GetPlayingSourcePath() const;

  //! Where queued items go.
  enum class Placement
  {
    //! After the rest.
    End,
    //! Next, if the playlist is playing; otherwise after the rest.
    Next
  };

  /*!
   * \brief Add items to a playlist without starting it. On a fed playlist they always go next.
   * \return The position of the first of them, or -1 if there were none.
   */
  int Queue(KODI::PLAYLIST::Type type, const CFileItemList& items, Placement placement);
  int Queue(KODI::PLAYLIST::Type type,
            const std::shared_ptr<CFileItem>& item,
            Placement placement = Placement::End);

  /*!
   * \brief Add items to a playlist at a position in list order, or at the end for -1.
   */
  void Insert(KODI::PLAYLIST::Type type, const CFileItemList& items, int position);

  /*!
   * \brief Remove the entry at a position in list order. The entry being played, and the one the
   * player has accepted to play next, stay.
   * \return false if it is one of those.
   */
  bool Remove(KODI::PLAYLIST::Type type, int position);

  /*!
   * \brief Remove every entry for this path, except the entry being played and the one the player
   * has accepted to play next.
   */
  void Remove(KODI::PLAYLIST::Type type, const std::string& path);

  /*!
   * \brief Replace the item of every entry for this item's path with a copy of it. Items on a
   * playlist are shared with whoever reads them, so none is changed in place.
   */
  void UpdateItem(KODI::PLAYLIST::Type type, const CFileItem& item);
  //! Give one entry a copy of the item, leaving other entries for the same path alone.
  void ReplaceItem(KODI::PLAYLIST::Type type, KODI::PLAYLIST::EntryId entry, const CFileItem& item);

  /*!
   * \brief Remove the entries on optical discs from both playlists.
   */
  void RemoveDiscItems();

  bool Swap(KODI::PLAYLIST::Type type, int position1, int position2);
  bool Move(KODI::PLAYLIST::Type type, int from, int to);

  /*!
   * \brief Empty a playlist. What is playing from it plays on to its end.
   */
  void Clear(KODI::PLAYLIST::Type type);

  /*!
   * \brief Replace a playlist's contents with the items, without playing it. The same items
   * rearranged keep their entries; otherwise the entry for the item that was current, at the same
   * start offset, becomes current.
   */
  void Replace(KODI::PLAYLIST::Type type, const CFileItemList& items);

  /*!
   * \brief Move the playing playlist on and play what follows.
   * \param advance Automatic when the current entry ended by itself.
   */
  Step PlayNext(KODI::PLAYLIST::Advance advance = KODI::PLAYLIST::Advance::User);
  Step PlayPrevious();

  /*!
   * \brief The file the player was playing ended by itself: play what follows, or close the
   * player when nothing does. An EPG playlist item holds the player open instead.
   * \return false if the player is held open for the item.
   */
  bool OnEntryEnded(const CFileItem& ended);

  /*!
   * \brief Make this the playlist the player works through, and move it on or back.
   */
  Step PlayNext(KODI::PLAYLIST::Type type);
  Step PlayPrevious(KODI::PLAYLIST::Type type);

  /*!
   * \brief Play the entry the given number of entries ahead of the current one in play order,
   * or behind it for a negative offset.
   */
  Step PlayOffset(int offset);

  /*!
   * \brief As PlayOffset(offset) on this playlist, switching to it first; when nothing is playing,
   * the playlist starts at that position instead.
   */
  Step PlayOffset(KODI::PLAYLIST::Type type, int offset);

  //! The application opened a file the playlists did not choose, so nothing is handed on.
  void ClearQueued();

  /*!
   * \return What the player has open: recorded on every start, including an entry handed on after
   * it left the playlist, and kept after playback ends until ResetCurrentItem(). An empty item
   * before anything has played.
   */
  std::shared_ptr<CFileItem> GetCurrentItem() const;
  void ResetCurrentItem();

  /*!
   * \brief Set how a playlist plays. Shuffling always draws a new order.
   * \param persist Yes when the user chose it, to keep it as their setting for next time; No for
   * restoring that setting or an override such as a disc that must not shuffle.
   */
  void SetShuffle(KODI::PLAYLIST::Type type, bool shuffle, Persist persist);
  bool IsShuffled(KODI::PLAYLIST::Type type) const;

  void SetRepeat(KODI::PLAYLIST::Type type, KODI::PLAYLIST::Repeat repeat, Persist persist);
  KODI::PLAYLIST::Repeat GetRepeat(KODI::PLAYLIST::Type type) const;

  void ToggleShuffle(KODI::PLAYLIST::Type type, Persist persist);

  /*!
   * \brief Move repeat on to the next state: Off, All, One, Off.
   */
  void CycleRepeat(KODI::PLAYLIST::Type type, Persist persist);

  //! Put back each playlist's saved shuffle and repeat, once the settings have loaded them.
  void RestoreSavedPlayOrder();

  /*!
   * \brief Replace a playlist's contents with what a feed places, and play it. One feed plays at a
   * time, so the other playlist's is dropped.
   * \param repeat The repeat the fed playlist plays with. Shuffle is off; both last only as long as
   * the feed, and the saved settings come back when it goes.
   * \return false if the feed placed nothing.
   */
  bool PlayFeed(KODI::PLAYLIST::Type type,
                std::shared_ptr<KODI::PLAYLIST::IFeed> feed,
                KODI::PLAYLIST::Repeat repeat);

  //! The playlist that has a feed, if one does.
  std::optional<KODI::PLAYLIST::Type> GetFedType() const;

  /*!
   * \brief Drop the feed and clear its playlist. What is playing from it plays on.
   */
  void DropFeed();

  //! How many items the feed held; -1 while there is no feed.
  int GetFeedTotal() const;
  //! How many items the feed has not placed yet; -1 while there is no feed.
  int GetFeedLeft() const;

  //! How a repeat state is written: on its own ("One"), or with the setting's name ("Repeat: One").
  enum class RepeatWording
  {
    State,
    WithSetting
  };

  //! \return The id of the string for a repeat state.
  static uint32_t RepeatLabel(KODI::PLAYLIST::Repeat repeat, RepeatWording wording);

  void ClearPlayLists();

  /*!
   * \brief Empty both playlists and work through neither, as for another profile.
   */
  void Reset();

  /*!
   * \brief Turn a selection into playlist entries: folders and playlist files are opened through
   * the directory layer, each only once; internet streams and playable plugin items become entries
   * as they are, and every other file the rules accept does.
   * \param startAt The item to start playing at, if any.
   * \return The position in entries of the item to start at; none if it did not become one.
   */
  static std::optional<int> ExpandToEntries(const std::shared_ptr<CFileItem>& item,
                                            KODI::PLAYLIST::IEntryRules& rules,
                                            std::shared_ptr<CFileItem> startAt,
                                            CFileItemList& entries);

  void SetObserver(IObserver* observer);
  void SetGUIListener(IGUIListener* listener);

  /*!
   * \brief The picture slideshow keeps its own list and cursor, and holds Video while it runs.
   */
  void SetSlideShowRunning(bool running);
  bool IsSlideShowRunning() const;

protected:
  KODI::PLAYLIST::CPlayList& EditPlayList(KODI::PLAYLIST::Type type);

  /*!
   * \brief Add to entries the items that can become entries (PLAYLIST::CanBeEntry), in order.
   * \return The position among those kept of the item at position in items; none if it is not kept.
   */
  static std::optional<int> EntriesOf(const CFileItemList& items,
                                      std::optional<int> position,
                                      CFileItemList& entries);

  //! Whether previous and next have nowhere to go: a channel, or one entry that does not repeat.
  bool IsSingleItemNonRepeatPlaylist() const;

  /*!
   * \brief The player accepted an entry to start by itself once the current one ends.
   */
  void OnNextQueued(KODI::PLAYLIST::EntryId entry);

  /*!
   * \brief The player could not take the next entry, so move on to it without playing it.
   */
  void SkipQueued(KODI::PLAYLIST::EntryId entry);

private:
  //! The entries of a playlist nothing may remove: the one playing and the one handed on.
  std::vector<KODI::PLAYLIST::EntryId> GetKeptEntries(KODI::PLAYLIST::Type type) const;

  /*!
   * \brief Offer the player the entry that plays next, for it to start by itself once the current
   * one ends.
   */
  void QueueNextEntry();
  /*!
   * \brief Report the repeat state if it differs from what was last reported.
   * \return Whether it did.
   */
  bool ReportRepeat(KODI::PLAYLIST::Type type);
  void ReportPlayListsChanged() const;
  //! Put back the playlist's saved shuffle and repeat, as when a feed goes.
  void RestoreSavedPlayOrder(KODI::PLAYLIST::Type type);

  /*!
   * \brief Keep a fed playlist placed some entries ahead of the current one, taking from its feed
   * outside the playlist's lock. A playlist that wraps starts its feed over when it runs out.
   */
  void TopUp(KODI::PLAYLIST::Type type);
  //! The feed of the fed playlist, if one has a feed.
  std::shared_ptr<KODI::PLAYLIST::IFeed> GetFeed() const;
  void TopUpOnce(KODI::PLAYLIST::Type type);
  void OnPlayListChanged(KODI::PLAYLIST::Type type,
                         const std::vector<KODI::PLAYLIST::PlayListChange>& changes);
  /*!
   * \brief Play an entry, opening it first if it is a playlist file; an entry that fails hands on
   * as the options say.
   * \param startsRun Whether this starts a run of the playlist rather than continuing one; a run
   * starts with the first entry that plays, so a failure hands it on.
   */
  bool PlayEntry(KODI::PLAYLIST::Type type,
                 KODI::PLAYLIST::EntryId entry,
                 const PlayOptions& options,
                 KODI::APPLICATION::StartsRun startsRun);
  Step PlayNextEntry(KODI::PLAYLIST::Advance advance,
                     KODI::APPLICATION::StartsRun startsRun,
                     const PlayOptions& options = {});
  Step PlayPreviousEntry(KODI::APPLICATION::StartsRun startsRun);
  void EndPlayback(bool clearPlayList);
  void ChangePlayingType(std::optional<KODI::PLAYLIST::Type> type);

  /*!
   * \brief What the player started: the entry OnNextQueued() handed it, now current, or else the
   * item it reports.
   */
  std::shared_ptr<CFileItem> OnStarted(const CGUIMessage& message);
  bool IsPlayingChannel() const;

  bool StartPlaying(KODI::PLAYLIST::Type type,
                    std::optional<int> position,
                    const PlayOptions& options,
                    bool newContents);

  const std::unique_ptr<IPlayback> m_playback;
  std::array<std::unique_ptr<KODI::PLAYLIST::CPlayList>, 2> m_playLists;

  mutable CCriticalSection m_critSection;
  std::optional<KODI::PLAYLIST::Type> m_playingType;
  std::array<KODI::PLAYLIST::Repeat, 2> m_reportedRepeat{KODI::PLAYLIST::Repeat::Off,
                                                         KODI::PLAYLIST::Repeat::Off};
  //! Set while SetRepeat() composes the state, so its intermediate steps are not reported.
  std::atomic<bool> m_composingRepeat{false};
  //! Set while PlayFeed() swaps feeds, so the change is reported once, when it is done.
  std::atomic<bool> m_composingFeed{false};
  //! Top-up requests per playlist; one runs at a time and serves those that arrive meanwhile.
  std::array<std::atomic<int>, 2> m_topUpRequests{};
  bool m_slideShowRunning{false};
  bool m_audioFollowsVideo{false};
  bool m_playbackStarted{false};
  KODI::PLAYLIST::EntryId m_queued{KODI::PLAYLIST::NO_ENTRY};
  std::shared_ptr<CFileItem> m_currentItem;
  std::atomic<IObserver*> m_observer{nullptr};
  std::atomic<IGUIListener*> m_guiListener{nullptr};
  int m_failedEntries{0};
  std::chrono::steady_clock::time_point m_failedEntriesStart;
};
