/*
 *  Copyright (C) 2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "AddonClass.h"

class CPictureInfoTag;

namespace XBMCAddon
{
namespace xbmc
{

///
/// \defgroup python_InfoTagPicture InfoTagPicture
/// \ingroup python_xbmc
/// @{
/// @brief **Kodi's picture info tag class.**
///
/// \python_class{ InfoTagPicture() }
///
/// Access and / or modify the picture metadata of a ListItem.
///
///-------------------------------------------------------------------------
/// @python_v20 New class added.
///
/// **Example:**
/// ~~~~~~~~~~~~~{.py}
/// ...
/// tag = item.getPictureInfoTag()
///
/// datetime_taken  = tag.getDateTimeTaken()
/// tag.setResolution(1920, 1080)
/// ...
/// ~~~~~~~~~~~~~
///
class InfoTagPicture : public AddonClass
{
private:
  CPictureInfoTag* infoTag;
  bool offscreen;
  bool owned;

public:
#ifndef SWIG
  explicit InfoTagPicture(const CPictureInfoTag* tag);
  explicit InfoTagPicture(CPictureInfoTag* tag, bool offscreen = false);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
  ///
  /// \ingroup python_InfoTagPicture
  /// @brief \python_func{ xbmc.InfoTagPicture([offscreen]) }
  /// Create a picture info tag.
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
  /// pictureinfo = xbmc.InfoTagPicture(offscreen=False)
  /// ...
  /// ~~~~~~~~~~~~~
  ///
  InfoTagPicture(...);
#else
  explicit InfoTagPicture(bool offscreen = false);
#endif
  ~InfoTagPicture() override;

#ifdef DOXYGEN_SHOULD_USE_THIS
  ///
  /// \ingroup python_InfoTagPicture
  /// @brief \python_func{ getResolution() }
  /// Get the resolution of the picture in the format "w x h".
  ///
  /// @return [string] Resolution of the picture in the format "w x h".
  ///
  ///
  ///-----------------------------------------------------------------------
  /// @python_v20 New function added.
  ///
  getResolution();
#else
  String getResolution();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
  ///
  /// \ingroup python_InfoTagPicture
  /// @brief \python_func{ getDateTimeTaken() }
  /// Get the date and time at which the picture was taken in W3C format.
  ///
  /// @return [string] Date and time at which the picture was taken in W3C format.
  ///
  ///
  ///-----------------------------------------------------------------------
  /// @python_v20 New function added.
  ///
  getDirector();
#else
  String getDateTimeTaken();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
  ///
  /// \ingroup python_InfoTagPicture
  /// @brief \python_func{ setResolution(width, height) }
  /// Sets the resolution of the picture.
  ///
  /// @param width              int - Width of the picture in pixels.
  /// @param height             int - Height of the picture in pixels.
  ///
  ///
  ///-----------------------------------------------------------------------
  /// @python_v20 New function added.
  ///
  setResolution(...);
#else
  void setResolution(int width, int height);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
  ///
  /// \ingroup python_InfoTagPicture
  /// @brief \python_func{ setDateTimeTaken(datetimetaken) }
  /// Sets the date and time at which the picture was taken in W3C format.
  /// The following formats are supported:
  /// - YYYY
  /// - YYYY-MM-DD
  /// - YYYY-MM-DDThh:mm[TZD]
  /// - YYYY-MM-DDThh:mm:ss[TZD]
  /// where the timezone (TZD) is always optional and can be in one of the
  /// following formats:
  /// - Z (for UTC)
  /// - +hh:mm
  /// - -hh:mm
  ///
  /// @param datetimetaken      string - Date and time at which the picture was taken in W3C format.
  ///
  ///
  ///-----------------------------------------------------------------------
  /// @python_v20 New function added.
  ///
  setDateTimeTaken(...);
#else
  void setDateTimeTaken(const String& datetimetaken);
#endif

#ifndef SWIG
  static void setResolutionRaw(CPictureInfoTag* infoTag, const String& resolution);
  static void setResolutionRaw(CPictureInfoTag* infoTag, int width, int height);
  static void setDateTimeTakenRaw(CPictureInfoTag* infoTag, String datetimetaken);
#endif
};

} // namespace xbmc
} // namespace XBMCAddon
