/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "AddonClass.h"
#include "Tuple.h"
#include "utils/StreamDetails.h"
#include "video/VideoInfoTag.h"

namespace XBMCAddon
{
  namespace xbmc
  {
    ///
    /// \defgroup python_xbmc_actor Actor
    /// \ingroup python_xbmc
    /// @{
    /// @brief **Actor class used in combination with InfoTagVideo.**
    ///
    /// \python_class{ xbmc.Actor([name, role, order, thumbnail]) }
    ///
    /// Represents a single actor in the cast of a video item wrapped by InfoTagVideo.
    ///
    ///
    ///-------------------------------------------------------------------------
    /// @python_v20 New class added.
    ///
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ...
    /// actor = xbmc.Actor('Sean Connery', 'James Bond', order=1)
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    class Actor : public AddonClass
    {
    public:
#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmc_actor Actor
      /// @brief \python_func{ xbmc.Actor([name, role, order, thumbnail]) }
      /// Creates a single actor for the cast of a video item wrapped by InfoTagVideo.
      ///
      /// @param name               [opt] string - Name of the actor.
      /// @param role               [opt] string - Role of the actor in the specific video item.
      /// @param order              [opt] integer - Order of the actor in the cast of the specific video item.
      /// @param thumbnail          [opt] string - Path / URL to the thumbnail of the actor.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// actor = xbmc.Actor('Sean Connery', 'James Bond', order=1)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      Actor(...);
#else
      explicit Actor(const String& name = emptyString,
                     const String& role = emptyString,
                     int order = -1,
                     const String& thumbnail = emptyString);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmc_actor
      /// @brief \python_func{ getName() }
      /// Get the name of the actor.
      ///
      /// @return [string] Name of the actor
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      getName();
#else
      String getName() const { return m_name; }
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmc_actor
      /// @brief \python_func{ getRole() }
      /// Get the role of the actor in the specific video item.
      ///
      /// @return [string] Role of the actor in the specific video item
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      getRole();
#else
      String getRole() const { return m_role; }
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmc_actor
      /// @brief \python_func{ getOrder() }
      /// Get the order of the actor in the cast of the specific video item.
      ///
      /// @return [integer] Order of the actor in the cast of the specific video item
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      getOrder();
#else
      int getOrder() const { return m_order; }
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmc_actor
      /// @brief \python_func{ getThumbnail() }
      /// Get the path / URL to the thumbnail of the actor.
      ///
      /// @return [string] Path / URL to the thumbnail of the actor
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      getThumbnail();
#else
      String getThumbnail() const { return m_thumbnail; }
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmc_actor
      /// @brief \python_func{ setName(name) }
      /// Set the name of the actor.
      ///
      /// @param name               string - Name of the actor.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setName(...);
#else
      void setName(const String& name) { m_name = name; }
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmc_actor
      /// @brief \python_func{ setRole(role) }
      /// Set the role of the actor in the specific video item.
      ///
      /// @param role               string - Role of the actor in the specific video item.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setRole(...);
#else
      void setRole(const String& role) { m_role = role; }
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmc_actor
      /// @brief \python_func{ setOrder(order) }
      /// Set the order of the actor in the cast of the specific video item.
      ///
      /// @param order              integer - Order of the actor in the cast of the specific video item.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setOrder(...);
#else
      void setOrder(int order) { m_order = order; }
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmc_actor
      /// @brief \python_func{ setThumbnail(thumbnail) }
      /// Set the path / URL to the thumbnail of the actor.
      ///
      /// @param thumbnail          string - Path / URL to the thumbnail of the actor.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setThumbnail(...);
#else
      void setThumbnail(const String& thumbnail) { m_thumbnail = thumbnail; }
#endif

#ifndef SWIG
      SActorInfo ToActorInfo() const;
#endif

    private:
      String m_name;
      String m_role;
      int m_order;
      String m_thumbnail;
    };
    /// @}

    ///
    /// \defgroup python_xbmc_videostreamdetail VideoStreamDetail
    /// \ingroup python_xbmc
    /// @{
    /// @brief **Video stream details class used in combination with InfoTagVideo.**
    ///
    /// \python_class{ xbmc.VideoStreamDetail([width, height, aspect, duration, codec, stereoMode, language, hdrType]) }
    ///
    /// Represents a single selectable video stream for a video item wrapped by InfoTagVideo.
    ///
    ///
    ///-------------------------------------------------------------------------
    /// @python_v20 New class added.
    ///
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ...
    /// videostream = xbmc.VideoStreamDetail(1920, 1080, language='English')
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    class VideoStreamDetail : public AddonClass
    {
    public:
#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmc_videostreamdetail
      /// @brief \python_func{ xbmc.VideoStreamDetail([width, height, aspect, duration, codec, stereomode, language, hdrtype]) }
      /// Creates a single video stream details class for a video item wrapped by InfoTagVideo.
      ///
      /// @param width              [opt] integer - Width of the video stream in pixel.
      /// @param height             [opt] integer - Height of the video stream in pixel.
      /// @param aspect             [opt] float - Aspect ratio of the video stream.
      /// @param duration           [opt] integer - Duration of the video stream in seconds.
      /// @param codec              [opt] string - Codec of the video stream.
      /// @param stereomode         [opt] string - Stereo mode of the video stream.
      /// @param language           [opt] string - Language of the video stream.
      /// @param hdrtype            [opt] string - HDR type of the video stream.
      ///                           The following types are supported:
      ///                           dolbyvision, hdr10, hlg
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// videostream = xbmc.VideoStreamDetail(1920, 1080, language='English')
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      VideoStreamDetail(...);
#else
      explicit VideoStreamDetail(int width = 0,
                                 int height = 0,
                                 float aspect = 0.0f,
                                 int duration = 0,
                                 const String& codec = emptyString,
                                 const String& stereomode = emptyString,
                                 const String& language = emptyString,
                                 const String& hdrtype = emptyString);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmc_videostreamdetail
      /// @brief \python_func{ getWidth() }
      /// Get the width of the video stream in pixel.
      ///
      /// @return [integer] Width of the video stream
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      getWidth();
#else
      int getWidth() const { return m_width; }
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmc_videostreamdetail
      /// @brief \python_func{ getHeight() }
      /// Get the height of the video stream in pixel.
      ///
      /// @return [integer] Height of the video stream
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      getHeight();
#else
      int getHeight() const { return m_height; }
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmc_videostreamdetail
      /// @brief \python_func{ getAspect() }
      /// Get the aspect ratio of the video stream.
      ///
      /// @return [float] Aspect ratio of the video stream
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      getAspect();
#else
      float getAspect() const { return m_aspect; }
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmc_videostreamdetail
      /// @brief \python_func{ getDuration() }
      /// Get the duration of the video stream in seconds.
      ///
      /// @return [float] Duration of the video stream in seconds
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      getDuration();
#else
      int getDuration() const { return m_duration; }
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmc_videostreamdetail
      /// @brief \python_func{ getCodec() }
      /// Get the codec of the stream.
      ///
      /// @return [string] Codec of the stream
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      getCodec();
#else
      String getCodec() const { return m_codec; }
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmc_videostreamdetail
      /// @brief \python_func{ getStereoMode() }
      /// Get the stereo mode of the video stream.
      ///
      /// @return [string] Stereo mode of the video stream
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      getStereoMode();
#else
      String getStereoMode() const { return m_stereoMode; }
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmc_videostreamdetail
      /// @brief \python_func{ getLanguage() }
      /// Get the language of the stream.
      ///
      /// @return [string] Language of the stream
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      getLanguage();
#else
      String getLanguage() const { return m_language; }
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmc_videostreamdetail
      /// @brief \python_func{ getHDRType() }
      /// Get the HDR type of the stream.
      ///
      /// @return [string] HDR type of the stream
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      getHDRType();
#else
      String getHDRType() const { return m_hdrType; }
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmc_videostreamdetail
      /// @brief \python_func{ setWidth(width) }
      /// Set the width of the video stream in pixel.
      ///
      /// @param width              integer - Width of the video stream in pixel.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setWidth(...);
#else
      void setWidth(int width) { m_width = width; }
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmc_videostreamdetail
      /// @brief \python_func{ setHeight(height) }
      /// Set the height of the video stream in pixel.
      ///
      /// @param height             integer - Height of the video stream in pixel.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setHeight(...);
#else
      void setHeight(int height) { m_height = height; }
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmc_videostreamdetail
      /// @brief \python_func{ setAspect(aspect) }
      /// Set the aspect ratio of the video stream.
      ///
      /// @param aspect             float - Aspect ratio of the video stream.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setAspect(...);
#else
      void setAspect(float aspect) { m_aspect = aspect; }
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmc_videostreamdetail
      /// @brief \python_func{ setDuration(duration) }
      /// Set the duration of the video stream in seconds.
      ///
      /// @param duration           integer - Duration of the video stream in seconds.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setDuration(...);
#else
      void setDuration(int duration) { m_duration = duration; }
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmc_videostreamdetail
      /// @brief \python_func{ setCodec(codec) }
      /// Set the codec of the stream.
      ///
      /// @param codec              string - Codec of the stream.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setCodec(...);
#else
      void setCodec(const String& codec) { m_codec = codec; }
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmc_videostreamdetail
      /// @brief \python_func{ setStereoMode(stereomode) }
      /// Set the stereo mode of the video stream.
      ///
      /// @param stereomode         string - Stereo mode of the video stream.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setStereoMode(...);
#else
      void setStereoMode(const String& stereomode) { m_stereoMode = stereomode; }
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmc_videostreamdetail
      /// @brief \python_func{ setLanguage(language) }
      /// Set the language of the stream.
      ///
      /// @param language           string - Language of the stream.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setLanguage(...);
#else
      void setLanguage(const String& language) { m_language = language; }
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmc_videostreamdetail
      /// @brief \python_func{ setHDRType(hdrtype) }
      /// Set the HDR type of the stream.
      ///
      /// @param hdrtype           string - HDR type of the stream.
      ///                          The following types are supported:
      ///                          dolbyvision, hdr10, hlg
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setHDRType(...);
#else
      void setHDRType(const String& hdrtype) { m_hdrType = hdrtype; }
#endif

#ifndef SWIG
      CStreamDetailVideo* ToStreamDetailVideo() const;
#endif

    private:
      int m_width;
      int m_height;
      float m_aspect;
      int m_duration;
      String m_codec;
      String m_stereoMode;
      String m_language;
      String m_hdrType;
    };
    /// @}

    ///
    /// \defgroup python_xbmc_audiostreamdetail AudioStreamDetail
    /// \ingroup python_xbmc
    /// @{
    /// @brief **Audio stream details class used in combination with InfoTagVideo.**
    ///
    /// \python_class{ xbmc.AudioStreamDetail([channels, codec, language]) }
    ///
    /// Represents a single selectable audio stream for a video item wrapped by InfoTagVideo.
    ///
    ///
    ///-------------------------------------------------------------------------
    /// @python_v20 New class added.
    ///
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ...
    /// audiostream = xbmc.AudioStreamDetail(6, 'DTS', 'English')
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    class AudioStreamDetail : public AddonClass
    {
    public:
#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmc_audiostreamdetail AudioStreamDetail
      /// @brief \python_func{ xbmc.AudioStreamDetail([channels, codec, language]) }
      /// Creates a single audio stream details class for a video item wrapped by InfoTagVideo.
      ///
      /// @param channels           [opt] integer - Number of channels in the audio stream.
      /// @param codec              [opt] string - Codec of the audio stream.
      /// @param language           [opt] string - Language of the audio stream.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// audiostream = xbmc.AudioStreamDetail(6, 'DTS', 'English')
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      AudioStreamDetail(...);
#else
      explicit AudioStreamDetail(int channels = -1,
                                 const String& codec = emptyString,
                                 const String& language = emptyString);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmc_audiostreamdetail
      /// @brief \python_func{ getChannels() }
      /// Get the number of channels in the stream.
      ///
      /// @return [integer] Number of channels in the stream
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      getChannels();
#else
      int getChannels() const { return m_channels; }
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmc_audiostreamdetail
      /// @brief \python_func{ getCodec() }
      /// Get the codec of the stream.
      ///
      /// @return [string] Codec of the stream
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      getCodec();
#else
      String getCodec() const { return m_codec; }
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmc_audiostreamdetail
      /// @brief \python_func{ getLanguage() }
      /// Get the language of the stream.
      ///
      /// @return [string] Language of the stream
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      getLanguage();
#else
      String getLanguage() const { return m_language; }
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmc_audiostreamdetail
      /// @brief \python_func{ setChannels(channels) }
      /// Set the number of channels in the stream.
      ///
      /// @param channels           integer - Number of channels in the stream.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setChannels(...);
#else
      void setChannels(int channels) { m_channels = channels; }
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmc_audiostreamdetail
      /// @brief \python_func{ setCodec(codec) }
      /// Set the codec of the stream.
      ///
      /// @param codec              string - Codec of the stream.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setCodec(...);
#else
      void setCodec(const String& codec) { m_codec = codec; }
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmc_audiostreamdetail
      /// @brief \python_func{ setLanguage(language) }
      /// Set the language of the stream.
      ///
      /// @param language           string - Language of the stream.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setLanguage(...);
#else
      void setLanguage(const String& language) { m_language = language; }
#endif

#ifndef SWIG
      CStreamDetailAudio* ToStreamDetailAudio() const;
#endif

    private:
      int m_channels;
      String m_codec;
      String m_language;
    };
    /// @}

    ///
    /// \defgroup python_xbmc_subtitlestreamdetail SubtitleStreamDetail
    /// \ingroup python_xbmc
    /// @{
    /// @brief **Subtitle stream details class used in combination with InfoTagVideo.**
    ///
    /// \python_class{ xbmc.SubtitleStreamDetail([language]) }
    ///
    /// Represents a single selectable subtitle stream for a video item wrapped by InfoTagVideo.
    ///
    ///
    ///-------------------------------------------------------------------------
    /// @python_v20 New class added.
    ///
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ...
    /// subtitlestream = xbmc.SubtitleStreamDetail('English')
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    class SubtitleStreamDetail : public AddonClass
    {
    public:
#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmc_subtitlestreamdetail SubtitleStreamDetail
      /// @brief \python_func{ xbmc.SubtitleStreamDetail([language]) }
      /// Creates a single subtitle stream details class for a video item wrapped by InfoTagVideo.
      ///
      /// @param language           [opt] string - Language of the subtitle.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// subtitlestream = xbmc.SubtitleStreamDetail('English')
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      SubtitleStreamDetail(...);
#else
      explicit SubtitleStreamDetail(const String& language = emptyString);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmc_subtitlestreamdetail
      /// @brief \python_func{ getLanguage() }
      /// Get the language of the stream.
      ///
      /// @return [string] Language of the stream
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      getLanguage();
#else
      String getLanguage() const { return m_language; }
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmc_subtitlestreamdetail
      /// @brief \python_func{ setLanguage(language) }
      /// Set the language of the stream.
      ///
      /// @param language           string - Language of the stream.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setLanguage(...);
#else
      void setLanguage(const String& language) { m_language = language; }
#endif

#ifndef SWIG
      CStreamDetailSubtitle* ToStreamDetailSubtitle() const;
#endif

    private:
      String m_language;
    };
    /// @}

    ///
    /// \defgroup python_InfoTagVideo InfoTagVideo
    /// \ingroup python_xbmc
    /// @{
    /// @brief **Kodi's video info tag class.**
    ///
    /// \python_class{ xbmc.InfoTagVideo([offscreen]) }
    ///
    /// Access and / or modify the video metadata of a ListItem.
    ///
    ///
    ///-------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ...
    /// tag = xbmc.Player().getVideoInfoTag()
    ///
    /// title = tag.getTitle()
    /// file  = tag.getFile()
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    class InfoTagVideo : public AddonClass
    {
    private:
      CVideoInfoTag* infoTag;
      bool offscreen;
      bool owned;

    public:
#ifndef SWIG
      explicit InfoTagVideo(const CVideoInfoTag* tag);
      explicit InfoTagVideo(CVideoInfoTag* tag, bool offscreen = false);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ xbmc.InfoTagVideo([offscreen]) }
      /// Create a video info tag.
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
      /// @python_v20 Added **offscreen** argument.
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// videoinfo = xbmc.InfoTagVideo(offscreen=False)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      InfoTagVideo(...);
#else
      explicit InfoTagVideo(bool offscreen = false);
#endif
      ~InfoTagVideo() override;

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getDbId() }
      /// Get identification number of tag in database
      ///
      /// @return [integer] database id
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v17 New function added.
      ///
      getDbId();
#else
      int getDbId();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getDirector() }
      /// Get [film director](https://en.wikipedia.org/wiki/Film_director)
      /// who has made the film (if present).
      ///
      /// @return [string] Film director name.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 Deprecated. Use **getDirectors()** instead.
      ///
      getDirector();
#else
      String getDirector();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getDirectors() }
      /// Get a list of [film directors](https://en.wikipedia.org/wiki/Film_director)
      /// who have made the film (if present).
      ///
      /// @return [list] List of film director names.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      getDirectors();
#else
      std::vector<String> getDirectors();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getWritingCredits() }
      /// Get the writing credits if present from video info tag.
      ///
      /// @return [string] Writing credits
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 Deprecated. Use **getWriters()** instead.
      ///
      getWritingCredits();
#else
      String getWritingCredits();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getWriters() }
      /// Get the list of writers (if present) from video info tag.
      ///
      /// @return [list] List of writers
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      getWriters();
#else
      std::vector<String> getWriters();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getGenre() }
      /// To get the [Video Genre](https://en.wikipedia.org/wiki/Film_genre)
      /// if available.
      ///
      /// @return [string] Genre name
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 Deprecated. Use **getGenres()** instead.
      ///
      getGenre();
#else
      String getGenre();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getGenres() }
      /// Get the list of [Video Genres](https://en.wikipedia.org/wiki/Film_genre)
      /// if available.
      ///
      /// @return [list] List of genres
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
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getTagLine() }
      /// Get video tag line if available.
      ///
      /// @return [string] Video tag line
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getTagLine();
#else
      String getTagLine();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getPlotOutline() }
      /// Get the outline plot of the video if present.
      ///
      /// @return [string] Outline plot
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getPlotOutline();
#else
      String getPlotOutline();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getPlot() }
      /// Get the plot of the video if present.
      ///
      /// @return [string] Plot
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getPlot();
#else
      String getPlot();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getPictureURL() }
      /// Get a picture URL of the video to show as screenshot.
      ///
      /// @return [string] Picture URL
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getPictureURL();
#else
      String getPictureURL();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getTitle() }
      /// Get the video title.
      ///
      /// @return [string] Video title
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getTitle();
#else
      String getTitle();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getTVShowTitle() }
      /// Get the video TV show title.
      ///
      /// @return [string] TV show title
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v17 New function added.
      ///
      getTVShowTitle();
#else
      String getTVShowTitle();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getMediaType() }
      /// Get the media type of the video.
      ///
      /// @return [string] media type
      ///
      /// Available strings about media type for video:
      /// | String         | Description                                       |
      /// |---------------:|:--------------------------------------------------|
      /// | video          | For normal video
      /// | set            | For a selection of video
      /// | musicvideo     | To define it as music video
      /// | movie          | To define it as normal movie
      /// | tvshow         | If this is it defined as tvshow
      /// | season         | The type is used as a series season
      /// | episode        | The type is used as a series episode
      ///
      ///-----------------------------------------------------------------------
      /// @python_v17 New function added.
      ///
      getMediaType();
#else
      String getMediaType();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getVotes() }
      /// Get the video votes if available from video info tag.
      ///
      /// @return [string] Votes
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 Deprecated. Use **getVotesAsInt()** instead.
      ///
      getVotes();
#else
      String getVotes();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getVotesAsInt([type]) }
      /// Get the votes of the rating (if available) as an integer.
      ///
      /// @param type           [opt] string - the type of the rating.
      /// - Some rating type values (any string possible):
      ///  | Label         | Type                                             |
      ///  |---------------|--------------------------------------------------|
      ///  | imdb          | string - type name
      ///  | tvdb          | string - type name
      ///  | tmdb          | string - type name
      ///  | anidb         | string - type name
      ///
      /// @return [integer] Votes
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      getVotesAsInt(type);
#else
      int getVotesAsInt(const String& type = "");
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getCast() }
      /// To get the cast of the video when available.
      ///
      /// @return [string] Video casts
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 Deprecated. Use **getActors()** instead.
      ///
      getCast();
#else
      String getCast();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getActors() }
      /// Get the cast of the video if available.
      ///
      /// @return [list] List of actors
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      getActors();
#else
      std::vector<Actor*> getActors();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getFile() }
      /// To get the video file name.
      ///
      /// @return [string] File name
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getFile();
#else
      String getFile();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getPath() }
      /// To get the path where the video is stored.
      ///
      /// @return [string] Path
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getPath();
#else
      String getPath();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getFilenameAndPath() }
      /// To get the full path with filename where the video is stored.
      ///
      /// @return [string] File name and Path
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v19 New function added.
      ///
      getFilenameAndPath();
#else
      String getFilenameAndPath();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getIMDBNumber() }
      /// To get the [IMDb](https://en.wikipedia.org/wiki/Internet_Movie_Database)
      /// number of the video (if present).
      ///
      /// @return [string] IMDb number
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getIMDBNumber();
#else
      String getIMDBNumber();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getSeason() }
      /// To get season number of a series
      ///
      /// @return [integer] season number
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v17 New function added.
      ///
      getSeason();
#else
      int getSeason();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getEpisode() }
      /// To get episode number of a series
      ///
      /// @return [integer] episode number
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v17 New function added.
      ///
      getEpisode();
#else
      int getEpisode();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getYear() }
      /// Get production year of video if present.
      ///
      /// @return [integer] Production Year
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getYear();
#else
      int getYear();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getRating([type]) }
      /// Get the video rating if present as float (double where supported).
      ///
      /// @param type           [opt] string - the type of the rating.
      /// - Some rating type values (any string possible):
      ///  | Label         | Type                                             |
      ///  |---------------|--------------------------------------------------|
      ///  | imdb          | string - type name
      ///  | tvdb          | string - type name
      ///  | tmdb          | string - type name
      ///  | anidb         | string - type name
      ///
      /// @return [float] The rating of the video
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 Optional `type` parameter added.
      ///
      getRating(type);
#else
      double getRating(const String& type = "");
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getUserRating() }
      /// Get the user rating if present as integer.
      ///
      /// @return [integer] The user rating of the video
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getUserRating();
#else
      int getUserRating();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getPlayCount() }
      /// To get the number of plays of the video.
      ///
      /// @return [integer] Play Count
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getPlayCount();
#else
      int getPlayCount();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getLastPlayed() }
      /// Get the last played date / time as string.
      ///
      /// @return [string] Last played date / time
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 Deprecated. Use **getLastPlayedAsW3C()** instead.
      ///
      getLastPlayed();
#else
      String getLastPlayed();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getLastPlayedAsW3C() }
      /// Get last played datetime as string in W3C format (YYYY-MM-DDThh:mm:ssTZD).
      ///
      /// @return [string] Last played datetime (W3C)
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      getLastPlayedAsW3C();
#else
      String getLastPlayedAsW3C();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getOriginalTitle() }
      /// To get the original title of the video.
      ///
      /// @return [string] Original title
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getOriginalTitle();
#else
      String getOriginalTitle();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getPremiered() }
      /// To get [premiered](https://en.wikipedia.org/wiki/Premiere) date
      /// of the video, if available.
      ///
      /// @return [string]
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 Deprecated. Use **getPremieredAsW3C()** instead.
      ///
      getPremiered();
#else
      String getPremiered();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getPremieredAsW3C() }
      /// Get [premiered](https://en.wikipedia.org/wiki/Premiere) date as string in W3C format (YYYY-MM-DD).
      ///
      /// @return [string] Premiered date (W3C)
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      getPremieredAsW3C();
#else
      String getPremieredAsW3C();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getFirstAired() }
      /// Returns first aired date as string from info tag.
      ///
      /// @return [string] First aired date
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 Deprecated. Use **getFirstAiredAsW3C()** instead.
      ///
      getFirstAired();
#else
      String getFirstAired();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getFirstAiredAsW3C() }
      /// Get first aired date as string in W3C format (YYYY-MM-DD).
      ///
      /// @return [string] First aired date (W3C)
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      getFirstAiredAsW3C();
#else
      String getFirstAiredAsW3C();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getTrailer() }
      /// To get the path where the trailer is stored.
      ///
      /// @return [string] Trailer path
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v17 New function added.
      ///
      getTrailer();
#else
      String getTrailer();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getArtist() }
      /// To get the artist name (for musicvideos)
      ///
      /// @return [std::vector<std::string>] Artist name
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v18 New function added.
      ///
      getArtist();
#else
      std::vector<std::string> getArtist();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getAlbum() }
      /// To get the album name (for musicvideos)
      ///
      /// @return [string] Album name
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v18 New function added.
      ///
      getAlbum();
#else
      String getAlbum();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getTrack() }
      /// To get the track number (for musicvideos)
      ///
      /// @return [int] Track number
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v18 New function added.
      ///
      getTrack();
#else
      int getTrack();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getDuration() }
      /// To get the duration
      ///
      /// @return [unsigned int] Duration
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v18 New function added.
      ///
      getDuration();
#else
      unsigned int getDuration();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getResumeTime()) }
      /// Gets the resume time of the video item.
      ///
      /// @return [double] Resume time
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      getResumeTime(...);
#else
      double getResumeTime();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getResumeTimeTotal()) }
      /// Gets the total duration stored with the resume time of the video item.
      ///
      /// @return [double] Total duration stored with the resume time
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      getResumeTimeTotal(...);
#else
      double getResumeTimeTotal();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ getUniqueID(key) }
      /// Get the unique ID of the given key.
      /// A unique ID is an identifier used by a (online) video database used to
      /// identify a video in its database.
      ///
      /// @param key            string - uniqueID name.
      /// - Some default uniqueID values (any string possible):
      ///  | Label         | Type                                             |
      ///  |---------------|--------------------------------------------------|
      ///  | imdb          | string - uniqueid name
      ///  | tvdb          | string - uniqueid name
      ///  | tmdb          | string - uniqueid name
      ///  | anidb         | string - uniqueid name
      ///
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      getUniqueID(key);
#else
      String getUniqueID(const char* key);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ setUniqueID(uniqueid, [type], [isdefault]) }
      /// Set the given unique ID.
      /// A unique ID is an identifier used by a (online) video database used to
      /// identify a video in its database.
      ///
      /// @param uniqueid           string - value of the unique ID.
      /// @param type               [opt] string - type / label of the unique ID.
      /// @param isdefault          [opt] bool - whether the given unique ID is the default unique ID.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setUniqueID(...);
#else
      void setUniqueID(const String& uniqueid, const String& type = "", bool isdefault = false);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ setUniqueIDs(values, defaultuniqueid) }
      /// Set the given unique IDs.
      /// A unique ID is an identifier used by a (online) video database used to
      /// identify a video in its database.
      ///
      /// @param values             dictionary - pairs of `{ 'label': 'value' }`.
      /// @param defaultuniqueid    [opt] string - the name of default uniqueID.
      ///
      ///  - Some example values (any string possible):
      ///  | Label         | Type                                              |
      ///  |:-------------:|:--------------------------------------------------|
      ///  | imdb          | string - uniqueid name
      ///  | tvdb          | string - uniqueid name
      ///  | tmdb          | string - uniqueid name
      ///  | anidb         | string - uniqueid name
      ///
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setUniqueIDs(...);
#else
      void setUniqueIDs(const std::map<String, String>& uniqueIDs,
                        const String& defaultuniqueid = "");
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ setDbId(dbid) }
      /// Set the database identifier of the video item.
      ///
      /// @param dbid               integer - Database identifier.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setDbId(...);
#else
      void setDbId(int dbid);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ setYear(year) }
      /// Set the year of the video item.
      ///
      /// @param year               integer - Year.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setYear(...);
#else
      void setYear(int year);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ setEpisode(episode) }
      /// Set the episode number of the episode.
      ///
      /// @param episode            integer - Episode number.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setEpisode(...);
#else
      void setEpisode(int episode);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ setSeason(season) }
      /// Set the season number of the video item.
      ///
      /// @param season             integer - Season number.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setSeason(...);
#else
      void setSeason(int season);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ setSortEpisode(sortepisode) }
      /// Set the episode sort number of the episode.
      ///
      /// @param sortepisode        integer - Episode sort number.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setSortEpisode(...);
#else
      void setSortEpisode(int sortepisode);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ setSortSeason(sortseason) }
      /// Set the season sort number of the season.
      ///
      /// @param sortseason         integer - Season sort number.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setSortSeason(...);
#else
      void setSortSeason(int sortseason);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ setEpisodeGuide(episodeguide) }
      /// Set the episode guide of the video item.
      ///
      /// @param episodeguide       string - Episode guide.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setEpisodeGuide(...);
#else
      void setEpisodeGuide(const String& episodeguide);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ setTop250(top250) }
      /// Set the top 250 number of the video item.
      ///
      /// @param top250             integer - Top 250 number.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setTop250(...);
#else
      void setTop250(int top250);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ setSetId(setid) }
      /// Set the movie set identifier of the video item.
      ///
      /// @param setid              integer - Set identifier.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setSetId(...);
#else
      void setSetId(int setid);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ setTrackNumber(tracknumber) }
      /// Set the track number of the music video item.
      ///
      /// @param tracknumber        integer - Track number.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setTrackNumber(...);
#else
      void setTrackNumber(int tracknumber);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ setRating(rating, [votes], [type], [isdefault]) }
      /// Set the rating of the video item.
      ///
      /// @param rating             float - Rating number.
      /// @param votes              integer - Number of votes.
      /// @param type               string - Type of the rating.
      /// @param isdefault          bool - Whether the rating is the default or not.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setRating(...);
#else
      void setRating(float rating, int votes = 0, const String& type = "", bool isdefault = false);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ setRatings(ratings, [defaultrating]) }
      /// Set the ratings of the video item.
      ///
      /// @param ratings            dictionary - `{ 'type': (rating, votes) }`.
      /// @param defaultrating      string - Type / Label of the default rating.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setRatings(...);
#else
      void setRatings(const std::map<String, Tuple<float, int>>& ratings,
                      const String& defaultrating = "");
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ setUserRating(userrating) }
      /// Set the user rating of the video item.
      ///
      /// @param userrating         integer - User rating.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setUserRating(...);
#else
      void setUserRating(int userrating);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ setPlaycount(playcount) }
      /// Set the playcount of the video item.
      ///
      /// @param playcount          integer - Playcount.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setPlaycount(...);
#else
      void setPlaycount(int playcount);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ setMpaa(mpaa) }
      /// Set the MPAA rating of the video item.
      ///
      /// @param mpaa               string - MPAA rating.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setMpaa(...);
#else
      void setMpaa(const String& mpaa);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ setPlot(plot) }
      /// Set the plot of the video item.
      ///
      /// @param plot               string - Plot.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setPlot(...);
#else
      void setPlot(const String& plot);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ setPlotOutline(plotoutline) }
      /// Set the plot outline of the video item.
      ///
      /// @param plotoutline        string - Plot outline.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setPlotOutline(...);
#else
      void setPlotOutline(const String& plotoutline);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ setTitle(title) }
      /// Set the title of the video item.
      ///
      /// @param title              string - Title.
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
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ setOriginalTitle(originaltitle) }
      /// Set the original title of the video item.
      ///
      /// @param originaltitle      string - Original title.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setOriginalTitle(...);
#else
      void setOriginalTitle(const String& originaltitle);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ setSortTitle(sorttitle) }
      /// Set the sort title of the video item.
      ///
      /// @param sorttitle          string - Sort title.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setSortTitle(...);
#else
      void setSortTitle(const String& sorttitle);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ setTagLine(tagline) }
      /// Set the tagline of the video item.
      ///
      /// @param tagline            string - Tagline.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setTagLine(...);
#else
      void setTagLine(const String& tagline);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ setTvShowTitle(tvshowtitle) }
      /// Set the TV show title of the video item.
      ///
      /// @param tvshowtitle        string - TV show title.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setTvShowTitle(...);
#else
      void setTvShowTitle(const String& tvshowtitle);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ setTvShowStatus(tvshowstatus) }
      /// Set the TV show status of the video item.
      ///
      /// @param status             string - TV show status.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setTvShowStatus(...);
#else
      void setTvShowStatus(const String& status);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ setGenres(genre) }
      /// Set the genres of the video item.
      ///
      /// @param genre              list - Genres.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setGenres(...);
#else
      void setGenres(std::vector<String> genre);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ setCountries(countries) }
      /// Set the countries of the video item.
      ///
      /// @param countries          list - Countries.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setCountries(...);
#else
      void setCountries(std::vector<String> countries);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ setDirectors(directors) }
      /// Set the directors of the video item.
      ///
      /// @param directors          list - Directors.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setDirectors(...);
#else
      void setDirectors(std::vector<String> directors);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ setStudios(studios) }
      /// Set the studios of the video item.
      ///
      /// @param studios            list - Studios.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setStudios(...);
#else
      void setStudios(std::vector<String> studios);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ setWriters(writers) }
      /// Set the writers of the video item.
      ///
      /// @param writers            list - Writers.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setWriters(...);
#else
      void setWriters(std::vector<String> writers);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ setDuration(duration) }
      /// Set the duration of the video item.
      ///
      /// @param duration           integer - Duration in seconds.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setDuration(...);
#else
      void setDuration(int duration);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ setPremiered(premiered) }
      /// Set the premiere date of the video item.
      ///
      /// @param premiered          string - Premiere date.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setPremiered(...);
#else
      void setPremiered(const String& premiered);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ setSet(set) }
      /// Set the movie set (name) of the video item.
      ///
      /// @param set                string - Movie set (name).
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setSet(...);
#else
      void setSet(const String& set);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ setSetOverview(setoverview) }
      /// Set the movie set overview of the video item.
      ///
      /// @param setoverview        string - Movie set overview.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setSetOverview(...);
#else
      void setSetOverview(const String& setoverview);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ setTags(tags) }
      /// Set the tags of the video item.
      ///
      /// @param tags               list - Tags.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setTags(...);
#else
      void setTags(std::vector<String> tags);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ setVideoAssetTitle(videoAssetTitle) }
      /// Set the video asset title of the item.
      ///
      /// @param videoAssetTitle     string - Video asset title.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v21 New function added.
      ///
      setVideoAssetTitle(...);
#else
      void setVideoAssetTitle(const String& videoAssetTitle);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ setProductionCode(const String& productioncode) }
      /// Set the production code of the video item.
      ///
      /// @param productioncode     string - Production code.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setProductionCode(...);
#else
      void setProductionCode(const String& productioncode);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ setFirstAired(firstaired) }
      /// Set the first aired date of the video item.
      ///
      /// @param firstaired         string - First aired date.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setFirstAired(...);
#else
      void setFirstAired(const String& firstaired);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ setLastPlayed(lastplayed) }
      /// Set the last played date of the video item.
      ///
      /// @param lastplayed         string - Last played date (YYYY-MM-DD HH:MM:SS).
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setLastPlayed(...);
#else
      void setLastPlayed(const String& lastplayed);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ setAlbum(album) }
      /// Set the album of the video item.
      ///
      /// @param album              string - Album.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setAlbum(...);
#else
      void setAlbum(const String& album);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ setVotes(votes) }
      /// Set the number of votes of the video item.
      ///
      /// @param votes              integer - Number of votes.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setVotes(...);
#else
      void setVotes(int votes);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ setTrailer(trailer) }
      /// Set the trailer of the video item.
      ///
      /// @param trailer            string - Trailer.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setTrailer(...);
#else
      void setTrailer(const String& trailer);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ setPath(path) }
      /// Set the path of the video item.
      ///
      /// @param path               string - Path.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setPath(...);
#else
      void setPath(const String& path);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ setFilenameAndPath(filenameandpath) }
      /// Set the filename and path of the video item.
      ///
      /// @param filenameandpath   string - Filename and path.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setFilenameAndPath(...);
#else
      void setFilenameAndPath(const String& filenameandpath);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ setIMDBNumber(imdbnumber) }
      /// Set the IMDb number of the video item.
      ///
      /// @param imdbnumber         string - IMDb number.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setIMDBNumber(...);
#else
      void setIMDBNumber(const String& imdbnumber);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ setDateAdded(dateadded) }
      /// Set the date added of the video item.
      ///
      /// @param dateadded          string - Date added (YYYY-MM-DD HH:MM:SS).
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setDateAdded(...);
#else
      void setDateAdded(const String& dateadded);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ setMediaType(mediatype) }
      /// Set the media type of the video item.
      ///
      /// @param mediatype          string - Media type.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setMediaType(...);
#else
      void setMediaType(const String& mediatype);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ setShowLinks(showlinks) }
      /// Set the TV show links of the movie.
      ///
      /// @param showlinks          list - TV show links.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setShowLinks(...);
#else
      void setShowLinks(std::vector<String> showlinks);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ setArtists(artists) }
      /// Set the artists of the music video item.
      ///
      /// @param artists            list - Artists.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setArtists(...);
#else
      void setArtists(std::vector<String> artists);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ setCast(actors) }
      /// Set the cast / actors of the video item.
      ///
      /// @param actors             list - Cast / Actors.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setCast(...);
#else
      void setCast(const std::vector<const Actor*>& actors);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ setResumePoint(time, [totaltime]) }
      /// Set the resume point of the video item.
      ///
      /// @param time               float - Resume point in seconds.
      /// @param totaltime          float - Total duration in seconds.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      setResumePoint(...);
#else
      void setResumePoint(double time, double totaltime = 0.0);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ addSeason(number, [name]) }
      /// Add a season with name. It needs at least the season number.
      ///
      /// @param number     int - the number of the season.
      /// @param name       string - the name of the season. Default "".
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// @python_v20 New function added.
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # addSeason(number, name))
      /// infotagvideo.addSeason(1, "Murder House")
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      addSeason(...);
#else
      void addSeason(int number, std::string name = "");
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ addSeasons(namedseasons) }
      /// Add named seasons to the TV show.
      ///
      /// @param namedseasons       list - `[ (season, name) ]`.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      addSeasons(...);
#else
      void addSeasons(const std::vector<Tuple<int, std::string>>& namedseasons);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ addVideoStream(stream) }
      /// Add a video stream to the video item.
      ///
      /// @param stream             VideoStreamDetail - Video stream.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      addVideoStream(...);
#else
      void addVideoStream(const VideoStreamDetail* stream);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ addAudioStream(stream) }
      /// Add an audio stream to the video item.
      ///
      /// @param stream             AudioStreamDetail - Audio stream.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      addAudioStream(...);
#else
      void addAudioStream(const AudioStreamDetail* stream);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ addSubtitleStream(stream) }
      /// Add a subtitle stream to the video item.
      ///
      /// @param stream             SubtitleStreamDetail - Subtitle stream.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      addSubtitleStream(...);
#else
      void addSubtitleStream(const SubtitleStreamDetail* stream);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagVideo
      /// @brief \python_func{ addAvailableArtwork(images) }
      /// Add an image to available artworks (needed for video scrapers)
      ///
      /// @param url            string - image path url
      /// @param arttype       string - image type
      /// @param preview        [opt] string - image preview path url
      /// @param referrer       [opt] string - referrer url
      /// @param cache          [opt] string - filename in cache
      /// @param post           [opt] bool - use post to retrieve the image (default false)
      /// @param isgz           [opt] bool - use gzip to retrieve the image (default false)
      /// @param season         [opt] integer - number of season in case of season thumb
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// infotagvideo.addAvailableArtwork(path_to_image_1, "thumb")
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      addAvailableArtwork(...);
#else
      void addAvailableArtwork(const std::string& url,
        const std::string& arttype = "",
        const std::string& preview = "",
        const std::string& referrer = "",
        const std::string& cache = "",
        bool post = false,
        bool isgz = false,
        int season = -1);
#endif
      /// @}

#ifndef SWIG
      static void setDbIdRaw(CVideoInfoTag* infoTag, int dbId);
      static void setUniqueIDRaw(CVideoInfoTag* infoTag,
                                 const String& uniqueID,
                                 const String& type = "",
                                 bool isDefault = false);
      static void setUniqueIDsRaw(CVideoInfoTag* infoTag,
                                  std::map<String, String> uniqueIDs,
                                  const String& defaultUniqueID = "");
      static void setYearRaw(CVideoInfoTag* infoTag, int year);
      static void setEpisodeRaw(CVideoInfoTag* infoTag, int episode);
      static void setSeasonRaw(CVideoInfoTag* infoTag, int season);
      static void setSortEpisodeRaw(CVideoInfoTag* infoTag, int sortEpisode);
      static void setSortSeasonRaw(CVideoInfoTag* infoTag, int sortSeason);
      static void setEpisodeGuideRaw(CVideoInfoTag* infoTag, const String& episodeGuide);
      static void setTop250Raw(CVideoInfoTag* infoTag, int top250);
      static void setSetIdRaw(CVideoInfoTag* infoTag, int setId);
      static void setTrackNumberRaw(CVideoInfoTag* infoTag, int trackNumber);
      static void setRatingRaw(CVideoInfoTag* infoTag,
                               float rating,
                               int votes = 0,
                               const std::string& type = "",
                               bool isDefault = false);
      static void setRatingsRaw(CVideoInfoTag* infoTag,
                                const std::map<String, Tuple<float, int>>& ratings,
                                const String& defaultRating = "");
      static void setUserRatingRaw(CVideoInfoTag* infoTag, int userRating);
      static void setPlaycountRaw(CVideoInfoTag* infoTag, int playcount);
      static void setMpaaRaw(CVideoInfoTag* infoTag, const String& mpaa);
      static void setPlotRaw(CVideoInfoTag* infoTag, const String& plot);
      static void setPlotOutlineRaw(CVideoInfoTag* infoTag, const String& plotOutline);
      static void setTitleRaw(CVideoInfoTag* infoTag, const String& title);
      static void setOriginalTitleRaw(CVideoInfoTag* infoTag, const String& originalTitle);
      static void setSortTitleRaw(CVideoInfoTag* infoTag, const String& sortTitle);
      static void setTagLineRaw(CVideoInfoTag* infoTag, const String& tagLine);
      static void setTvShowTitleRaw(CVideoInfoTag* infoTag, const String& tvshowTitle);
      static void setTvShowStatusRaw(CVideoInfoTag* infoTag, const String& tvshowStatus);
      static void setGenresRaw(CVideoInfoTag* infoTag, std::vector<String> genre);
      static void setCountriesRaw(CVideoInfoTag* infoTag, std::vector<String> countries);
      static void setDirectorsRaw(CVideoInfoTag* infoTag, std::vector<String> directors);
      static void setStudiosRaw(CVideoInfoTag* infoTag, std::vector<String> studios);
      static void setWritersRaw(CVideoInfoTag* infoTag, std::vector<String> writers);
      static void setDurationRaw(CVideoInfoTag* infoTag, int duration);
      static void setPremieredRaw(CVideoInfoTag* infoTag, const String& premiered);
      static void setSetRaw(CVideoInfoTag* infoTag, const String& set);
      static void setSetOverviewRaw(CVideoInfoTag* infoTag, const String& setOverview);
      static void setTagsRaw(CVideoInfoTag* infoTag, std::vector<String> tags);
      static void setVideoAssetTitleRaw(CVideoInfoTag* infoTag, const String& videoAssetTitle);
      static void setProductionCodeRaw(CVideoInfoTag* infoTag, const String& productionCode);
      static void setFirstAiredRaw(CVideoInfoTag* infoTag, const String& firstAired);
      static void setLastPlayedRaw(CVideoInfoTag* infoTag, const String& lastPlayed);
      static void setAlbumRaw(CVideoInfoTag* infoTag, const String& album);
      static void setVotesRaw(CVideoInfoTag* infoTag, int votes);
      static void setTrailerRaw(CVideoInfoTag* infoTag, const String& trailer);
      static void setPathRaw(CVideoInfoTag* infoTag, const String& path);
      static void setFilenameAndPathRaw(CVideoInfoTag* infoTag, const String& filenameAndPath);
      static void setIMDBNumberRaw(CVideoInfoTag* infoTag, const String& imdbNumber);
      static void setDateAddedRaw(CVideoInfoTag* infoTag, const String& dateAdded);
      static void setMediaTypeRaw(CVideoInfoTag* infoTag, const String& mediaType);
      static void setShowLinksRaw(CVideoInfoTag* infoTag, std::vector<String> showLinks);
      static void setArtistsRaw(CVideoInfoTag* infoTag, std::vector<String> artists);
      static void setCastRaw(CVideoInfoTag* infoTag, std::vector<SActorInfo> cast);
      static void setResumePointRaw(CVideoInfoTag* infoTag, double time, double totalTime = 0.0);

      static void addSeasonRaw(CVideoInfoTag* infoTag, int number, std::string name = "");
      static void addSeasonsRaw(CVideoInfoTag* infoTag,
                                const std::vector<Tuple<int, std::string>>& namedSeasons);

      static void addStreamRaw(CVideoInfoTag* infoTag, CStreamDetail* stream);
      static void finalizeStreamsRaw(CVideoInfoTag* infoTag);

      static void addAvailableArtworkRaw(CVideoInfoTag* infoTag,
                                         const std::string& url,
                                         const std::string& art_type = "",
                                         const std::string& preview = "",
                                         const std::string& referrer = "",
                                         const std::string& cache = "",
                                         bool post = false,
                                         bool isgz = false,
                                         int season = -1);
#endif
    };
  }
}
