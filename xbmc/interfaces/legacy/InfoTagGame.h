/*
 *  Copyright (C) 2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "AddonClass.h"

namespace KODI
{
namespace GAME
{
class CGameInfoTag;
}
} // namespace KODI

namespace XBMCAddon
{
namespace xbmc
{
///
/// \defgroup python_InfoTagGame InfoTagGame
/// \ingroup python_xbmc
/// @{
/// @brief **Kodi's game info tag class.**
///
/// \python_class{ InfoTagGame() }
///
/// Access and / or modify the game metadata of a ListItem.
///
///-------------------------------------------------------------------------
/// @python_v20 New class added.
///
/// **Example:**
/// ~~~~~~~~~~~~~{.py}
/// ...
/// tag = item.getGameInfoTag()
///
/// title = tag.getTitle()
/// tag.setDeveloper('John Doe')
/// ...
/// ~~~~~~~~~~~~~
///
class InfoTagGame : public AddonClass
{
private:
  KODI::GAME::CGameInfoTag* infoTag;
  bool offscreen;
  bool owned;

public:
#ifndef SWIG
  explicit InfoTagGame(const KODI::GAME::CGameInfoTag* tag);
  explicit InfoTagGame(KODI::GAME::CGameInfoTag* tag, bool offscreen = false);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
  ///
  /// \ingroup python_InfoTagGame
  /// @brief \python_func{ xbmc.InfoTagGame([offscreen]) }
  /// Create a game info tag.
  ///
  /// @param offscreen            [opt] bool (default `False`) - if GUI based locks should be
  ///                                          avoided. Most of the times listitems are created
  ///                                          offscreen and added later to a container
  ///                                          for display (e.g. plugins) or they are not
  ///                                          even displayed (e.g. python scrapers).
  ///                                          In such cases, there is no need to lock the
  ///                                          GUI when creating the items (increasing your addon
  ///                                          performance).
  ///                                          Note however, that if you are creating listitems
  ///                                          and managing the container itself (e.g using
  ///                                          WindowXML or WindowXMLDialog classes) subsquent
  ///                                          modifications to the item will require locking.
  ///                                          Thus, in such cases, use the default value (`False`).
  ///
  ///
  ///-----------------------------------------------------------------------
  /// @python_v20 New function added.
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.py}
  /// ...
  /// gameinfo = xbmc.InfoTagGame(offscreen=False)
  /// ...
  /// ~~~~~~~~~~~~~
  ///
  InfoTagGame(...);
#else
  explicit InfoTagGame(bool offscreen = false);
#endif
  ~InfoTagGame() override;

#ifdef DOXYGEN_SHOULD_USE_THIS
  ///
  /// \ingroup python_InfoTagGame
  /// @brief \python_func{ getTitle() }
  /// Gets the title of the game.
  ///
  /// @return [string] title
  ///
  ///
  ///-----------------------------------------------------------------------
  /// @python_v20 New function added.
  ///
  getTitle();
#else
  String getTitle();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
  ///
  /// \ingroup python_InfoTagGame
  /// @brief \python_func{ getPlatform() }
  /// Gets the platform on which the game is run.
  ///
  /// @return [string] platform
  ///
  ///
  ///-----------------------------------------------------------------------
  /// @python_v20 New function added.
  ///
  getPlatform();
#else
  String getPlatform();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
  ///
  /// \ingroup python_InfoTagGame
  /// @brief \python_func{ getGenres() }
  /// Gets the genres of the game.
  ///
  /// @return [list] genres
  ///
  ///
  ///-----------------------------------------------------------------------
  /// @python_v20 New function added.
  ///
  getGenres();
#else
  std::vector<String> getGenres();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
  ///
  /// \ingroup python_InfoTagGame
  /// @brief \python_func{ getPublisher() }
  /// Gets the publisher of the game.
  ///
  /// @return [string] publisher
  ///
  ///
  ///-----------------------------------------------------------------------
  /// @python_v20 New function added.
  ///
  getPublisher();
#else
  String getPublisher();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
  ///
  /// \ingroup python_InfoTagGame
  /// @brief \python_func{ getDeveloper() }
  /// Gets the developer of the game.
  ///
  /// @return [string] developer
  ///
  ///
  ///-----------------------------------------------------------------------
  /// @python_v20 New function added.
  ///
  getDeveloper();
#else
  String getDeveloper();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
  ///
  /// \ingroup python_InfoTagGame
  /// @brief \python_func{ getOverview() }
  /// Gets the overview of the game.
  ///
  /// @return [string] overview
  ///
  ///
  ///-----------------------------------------------------------------------
  /// @python_v20 New function added.
  ///
  getOverview();
#else
  String getOverview();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
  ///
  /// \ingroup python_InfoTagGame
  /// @brief \python_func{ getYear() }
  /// Gets the year in which the game was published.
  ///
  /// @return [integer] year
  ///
  ///
  ///-----------------------------------------------------------------------
  /// @python_v20 New function added.
  ///
  getYear();
#else
  unsigned int getYear();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
  ///
  /// \ingroup python_InfoTagGame
  /// @brief \python_func{ getGameClient() }
  /// Gets the add-on ID of the game client executing the game.
  ///
  /// @return [string] game client
  ///
  ///
  ///-----------------------------------------------------------------------
  /// @python_v20 New function added.
  ///
  getGameClient();
#else
  String getGameClient();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
  ///
  /// \ingroup python_InfoTagGame
  /// @brief \python_func{ setTitle(title) }
  /// Sets the title of the game.
  ///
  /// @param title              string - title.
  ///
  ///
  ///-----------------------------------------------------------------------
  /// @python_v20 New function added.
  ///
  setTitle(...);
#else
  void setTitle(const String& title);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
  ///
  /// \ingroup python_InfoTagGame
  /// @brief \python_func{ setPlatform(platform) }
  /// Sets the platform on which the game is run.
  ///
  /// @param platform           string - platform.
  ///
  ///
  ///-----------------------------------------------------------------------
  /// @python_v20 New function added.
  ///
  setPlatform(...);
#else
  void setPlatform(const String& platform);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
  ///
  /// \ingroup python_InfoTagGame
  /// @brief \python_func{ setGenres(genres) }
  /// Sets the genres of the game.
  ///
  /// @param genres             list - genres.
  ///
  ///
  ///-----------------------------------------------------------------------
  /// @python_v20 New function added.
  ///
  setGenres(...);
#else
  void setGenres(const std::vector<String>& genres);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
  ///
  /// \ingroup python_InfoTagGame
  /// @brief \python_func{ setPublisher(publisher) }
  /// Sets the publisher of the game.
  ///
  /// @param publisher          string - publisher.
  ///
  ///
  ///-----------------------------------------------------------------------
  /// @python_v20 New function added.
  ///
  setPublisher(...);
#else
  void setPublisher(const String& publisher);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
  ///
  /// \ingroup python_InfoTagGame
  /// @brief \python_func{ setDeveloper(developer) }
  /// Sets the developer of the game.
  ///
  /// @param developer          string - title.
  ///
  ///
  ///-----------------------------------------------------------------------
  /// @python_v20 New function added.
  ///
  setDeveloper(...);
#else
  void setDeveloper(const String& developer);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
  ///
  /// \ingroup python_InfoTagGame
  /// @brief \python_func{ setOverview(overview) }
  /// Sets the overview of the game.
  ///
  /// @param overview           string - overview.
  ///
  ///
  ///-----------------------------------------------------------------------
  /// @python_v20 New function added.
  ///
  setOverview(...);
#else
  void setOverview(const String& overview);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
  ///
  /// \ingroup python_InfoTagGame
  /// @brief \python_func{ setYear(year) }
  /// Sets the year in which the game was published.
  ///
  /// @param year               integer - year.
  ///
  ///
  ///-----------------------------------------------------------------------
  /// @python_v20 New function added.
  ///
  setYear(...);
#else
  void setYear(unsigned int year);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
  ///
  /// \ingroup python_InfoTagGame
  /// @brief \python_func{ setGameClient(gameClient) }
  /// Sets the add-on ID of the game client executing the game.
  ///
  /// @param gameClient         string - game client.
  ///
  ///
  ///-----------------------------------------------------------------------
  /// @python_v20 New function added.
  ///
  setGameClient(...);
#else
  void setGameClient(const String& gameClient);
#endif

#ifndef SWIG
  static void setTitleRaw(KODI::GAME::CGameInfoTag* infoTag, const String& title);
  static void setPlatformRaw(KODI::GAME::CGameInfoTag* infoTag, const String& platform);
  static void setGenresRaw(KODI::GAME::CGameInfoTag* infoTag, const std::vector<String>& genres);
  static void setPublisherRaw(KODI::GAME::CGameInfoTag* infoTag, const String& publisher);
  static void setDeveloperRaw(KODI::GAME::CGameInfoTag* infoTag, const String& developer);
  static void setOverviewRaw(KODI::GAME::CGameInfoTag* infoTag, const String& overview);
  static void setYearRaw(KODI::GAME::CGameInfoTag* infoTag, unsigned int year);
  static void setGameClientRaw(KODI::GAME::CGameInfoTag* infoTag, const String& gameClient);
#endif
};

} // namespace xbmc
} // namespace XBMCAddon
