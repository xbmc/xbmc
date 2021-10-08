/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDSubtitlesLibass.h"

#include "ServiceBroker.h"
#include "cores/VideoPlayer/Interface/TimingConstants.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "threads/SingleLock.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <cstring>

using namespace KODI::SUBTITLES;

namespace
{
constexpr int BORDER_STYLE_OUTLINE = 1; // ASS_Style->BorderStyle: Outline + drop shadow
constexpr int BORDER_STYLE_OUTLINE_BOX = 4; // ASS_Style->BorderStyle: Outline + box

// Directory where user defined fonts are located (and where mkv fonts are extracted to)
constexpr const char* userFontPath = "special://home/media/Fonts/";
// Directory where Kodi bundled fonts (default ones like Arial or Teletext) are located
constexpr const char* systemFontPath = "special://xbmc/media/Fonts/";

std::string GetDefaultFontPath(std::string& font)
{
  constexpr std::array<const char*, 2> fontSources{userFontPath, systemFontPath};

  for (const auto& path : fontSources)
  {
    auto fontPath = URIUtils::AddFileToFolder(path, font);
    if (XFILE::CFile::Exists(fontPath))
    {
      return CSpecialProtocol::TranslatePath(fontPath).c_str();
    }
  }
  CLog::Log(LOGERROR, "CDVDSubtitlesLibass: Could not find font {} in font sources", font);
  return "";
}

// Convert RGB/ARGB to RGBA by appling also the opacity value
UTILS::Color ConvColor(UTILS::Color argbColor, int opacity = 100)
{
  return ColorUtils::ConvertToRGBA(
      ColorUtils::ChangeOpacity(argbColor, (100.0f - opacity) / 100.0f));
}

} // namespace

static void libass_log(int level, const char* fmt, va_list args, void* data)
{
  if (level >= 5)
    return;
  std::string log = StringUtils::FormatV(fmt, args);
  CLog::Log(LOGDEBUG, "CDVDSubtitlesLibass: [ass] {}", log);
}

CDVDSubtitlesLibass::CDVDSubtitlesLibass()
{
  CLog::Log(LOGINFO, "CDVDSubtitlesLibass: Using libass version {0:x}", ass_library_version());
  CLog::Log(LOGINFO, "CDVDSubtitlesLibass: Creating ASS library structure");
  m_library = ass_library_init();
  if (!m_library)
    return;

  ass_set_message_cb(m_library, libass_log, this);

  CLog::Log(LOGINFO, "CDVDSubtitlesLibass: Initializing ASS Renderer");

  m_renderer = ass_renderer_init(m_library);

  if (!m_renderer)
    throw std::runtime_error("Libass render failed to initialize");
}

CDVDSubtitlesLibass::~CDVDSubtitlesLibass()
{
  if (m_track)
    ass_free_track(m_track);
  ass_renderer_done(m_renderer);
  ass_library_done(m_library);
}

void CDVDSubtitlesLibass::Configure()
{
  CLog::Log(LOGINFO, "CDVDSubtitlesLibass: Initializing ASS library font settings");

  if (!m_renderer)
  {
    CLog::Log(LOGERROR, "CDVDSubtitlesLibass: Failed to initialize ASS font settings. ASS renderer "
                        "not initialized.");
    return;
  }

  m_subtitleType = NATIVE;
  ass_set_margins(m_renderer, 0, 0, 0, 0);
  ass_set_use_margins(m_renderer, 0);

  // libass uses fontconfig (system lib) by default in some platforms (e.g. linux/android) or as
  // a fallback for all platforms. It is not wrapped so translate the path before calling into libass
  ass_set_fonts_dir(m_library, CSpecialProtocol::TranslatePath(userFontPath).c_str());
  ass_set_font_scale(m_renderer, 1);

  // Extract font must be set before loading ASS/SSA data,
  // after that cannot be changed
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  bool overrideFont = settings->GetBool(CSettings::SETTING_SUBTITLES_OVERRIDEASSFONTS);
  ass_set_extract_fonts(m_library, overrideFont ? 0 : 1);
}

bool CDVDSubtitlesLibass::DecodeHeader(char* data, int size)
{
  CSingleLock lock(m_section);
  if (!m_library || !data)
    return false;

  CLog::Log(LOGINFO, "CDVDSubtitlesLibass: Creating new ASS track");
  m_track = ass_new_track(m_library);

  ass_process_codec_private(m_track, data, size);
  return true;
}

bool CDVDSubtitlesLibass::DecodeDemuxPkt(const char* data, int size, double start, double duration)
{
  CSingleLock lock(m_section);
  if (!m_track)
  {
    CLog::Log(LOGERROR, "{} - No SSA header found.", __FUNCTION__);
    return false;
  }

  //! @bug libass isn't const correct
  ass_process_chunk(m_track, const_cast<char*>(data), size, DVD_TIME_TO_MSEC(start),
                    DVD_TIME_TO_MSEC(duration));
  return true;
}

bool CDVDSubtitlesLibass::CreateTrack()
{
  CSingleLock lock(m_section);
  if (!m_library)
  {
    CLog::Log(LOGERROR, "{} - Failed to create ASS track, library not initialized.", __FUNCTION__);
    return false;
  }

  CLog::Log(LOGINFO, "CDVDSubtitlesLibass: Creating new ASS track");
  m_track = ass_new_track(m_library);
  if (m_track == NULL)
  {
    CLog::Log(LOGERROR, "{} - Failed to allocate ASS track.", __FUNCTION__);
    return false;
  }

  m_track->track_type = m_track->TRACK_TYPE_ASS;
  m_track->Timer = 100.;
  // Set fixed values to PlayRes to allow the use of style override code for positioning
  m_track->PlayResX = (int)VIEWPORT_WIDTH;
  m_track->PlayResY = (int)VIEWPORT_HEIGHT;
  m_track->Kerning = true; // Font kerning improves the letterspacing
  m_track->WrapStyle = 1; // The line feed \n doesn't break but wraps (instead \N breaks)

  return true;
}

bool CDVDSubtitlesLibass::CreateStyle()
{
  CSingleLock lock(m_section);
  if (!m_library)
  {
    CLog::Log(LOGERROR, "{} - Failed to create ASS style, library not initialized.", __FUNCTION__);
    return false;
  }

  if (!m_track)
  {
    CLog::Log(LOGERROR, "{} - Failed to create ASS style, track not initialized.", __FUNCTION__);
    return false;
  }

  m_defaultKodiStyleId = ass_alloc_style(m_track);
  return true;
}

bool CDVDSubtitlesLibass::CreateTrack(char* buf, size_t size)
{
  CSingleLock lock(m_section);
  if (!m_library)
  {
    CLog::Log(LOGERROR, "{} - No ASS library struct (m_library)", __FUNCTION__);
    return false;
  }

  CLog::Log(LOGINFO, "CDVDSubtitlesLibass: Creating m_track from SSA buffer");

  m_track = ass_read_memory(m_library, buf, size, 0);
  if (m_track == NULL)
    return false;

  return true;
}

ASS_Image* CDVDSubtitlesLibass::RenderImage(double pts,
                                            renderOpts opts,
                                            bool updateStyle,
                                            std::shared_ptr<struct KODI::SUBTITLES::style> subStyle,
                                            int* changes)
{
  CSingleLock lock(m_section);
  if (!m_renderer || !m_track)
  {
    CLog::Log(LOGERROR, "{} - ASS renderer/ASS track not initialized.", __FUNCTION__);
    return NULL;
  }

  if (!subStyle)
  {
    CLog::Log(LOGERROR, "{} - The subtitle overlay style is not set.", __FUNCTION__);
    return NULL;
  }

  if (updateStyle || m_currentDefaultStyleId == ASS_NO_ID)
  {
    ApplyStyle(*subStyle.get(), opts);
    m_drawWithinBlackBars = subStyle->drawWithinBlackBars;
  }

  double sar = (double)opts.sourceWidth / opts.sourceHeight;
  double dar = (double)opts.videoWidth / opts.videoHeight;

  ass_set_frame_size(m_renderer, opts.frameWidth, opts.frameHeight);

  if (m_drawWithinBlackBars)
  {
    int marginTop = (opts.frameHeight - opts.videoHeight) / 2;
    int marginLeft = (opts.frameWidth - opts.videoWidth) / 2;
    ass_set_margins(m_renderer, marginTop, marginTop, marginLeft, marginLeft);
  }
  ass_set_use_margins(m_renderer, m_drawWithinBlackBars);

  // Vertical text position in percent (if 0 do nothing)
  ass_set_line_position(m_renderer, opts.position);

  ass_set_pixel_aspect(m_renderer, sar / dar);

  // For posterity ass_render_frame have an inconsistent rendering for overlapped subtitles cases,
  // if the playback occurs in sequence (without seeks) the overlapped subtitles lines will be rendered in right order
  // if you seek forward/backward the video, the overlapped subtitles lines could be rendered in the wrong order
  // this is a known side effect from libass devs and not a bug from our part
  return ass_render_frame(m_renderer, m_track, DVD_TIME_TO_MSEC(pts), changes);
}

void CDVDSubtitlesLibass::ApplyStyle(style subStyle, renderOpts opts)
{
  CLog::Log(LOGDEBUG, "{} - Start setting up the LibAss style", __FUNCTION__);

  ConfigureFont((m_subtitleType == NATIVE && subStyle.assOverrideFont), subStyle.fontName);

  ASS_Style* style = nullptr;

  if (m_subtitleType == ADAPTED ||
      (m_subtitleType == NATIVE && subStyle.assOverrideStyles != AssOverrideStyles::DISABLED))
  {
    m_currentDefaultStyleId = m_defaultKodiStyleId;

    if (m_subtitleType == NATIVE)
    {
      // ASS_Style is a POD struct need to be initialized with {}
      ASS_Style assStyle{};
      style = &assStyle;
    }
    else
    {
      style = &m_track->styles[m_currentDefaultStyleId];
    }

    style->Name = strdup("KodiDefault");

    // Calculate the scale (influence ASS style properties)
    double scale = 1.0;
    int playResY;
    if (m_subtitleType == NATIVE &&
        (subStyle.assOverrideStyles == AssOverrideStyles::STYLES ||
         subStyle.assOverrideStyles == AssOverrideStyles::STYLES_POSITIONS))
    {
      // With styles overrided the PlayResY will be changed to 288
      playResY = 288;
      scale = 288. / 720;
    }
    else
    {
      playResY = m_track->PlayResY;
    }

    // It is mandatory set the FontName, the text is case sensitive
    style->FontName = strdup(subStyle.fontName.c_str());

    if (m_subtitleType != NATIVE ||
        (m_subtitleType == NATIVE && subStyle.assOverrideStyles != AssOverrideStyles::POSITIONS))
    {
      // Configure the font properties
      // FIXME: The font size need to be scaled to be shown in right PT size
      style->FontSize = (subStyle.fontSize / 720) * playResY;
      // Modifies the width/height of the font (1 = 100%)
      style->ScaleX = 1.0;
      style->ScaleY = 1.0;
      // Extra space between characters causes the underlined
      // text line to become more discontinuous (test on LibAss 15.1)
      style->Spacing = 0;

      bool isFontBold =
          (subStyle.fontStyle == FontStyle::BOLD || subStyle.fontStyle == FontStyle::BOLD_ITALIC);
      bool isFontItalic =
          (subStyle.fontStyle == FontStyle::ITALIC || subStyle.fontStyle == FontStyle::BOLD_ITALIC);
      style->Bold = isFontBold * -1;
      style->Italic = isFontItalic * -1;

      // Compute the font color, depending on the opacity
      UTILS::Color subColor = ConvColor(subStyle.fontColor, subStyle.fontOpacity);
      // Set default subtitles color
      style->PrimaryColour = subColor;
      // Set SecondaryColour may be used to prevent an onscreen collision
      style->SecondaryColour = subColor;

      // Compute the outline color
      UTILS::Color outlineColor = ConvColor(subStyle.outlineColor);

      // Configure the effects
      if (subStyle.borderStyle == BorderStyle::OUTLINE)
      {
        style->BorderStyle = BORDER_STYLE_OUTLINE;
        style->OutlineColour = outlineColor;
        style->BackColour = ConvColor(UTILS::COLOR::BLACK); // Shadow color
        style->Outline = 1 * scale;
        style->Shadow = 0;
      }
      else
      {
        // Compute the color to be used for the box, depending on the opacity
        style->BorderStyle = BORDER_STYLE_OUTLINE_BOX;
        style->OutlineColour = outlineColor;
        style->BackColour = ConvColor(subStyle.backgroundColor, subStyle.backgroundOpacity);
        style->Outline = 1 * scale;
        style->Shadow = 4; // Space between the text and the box edges
      }

      // Set the margins (in pixel)
      style->MarginL = 20 * scale;
      style->MarginR = style->MarginL;
      // Vertical margin (direction depends on alignment)
      // to be set only when the video calibration position setting is not used
      if (opts.usePosition)
        style->MarginV = 0;
      else
        style->MarginV = 20 * scale;
    }

    // Set the alignment
    if (subStyle.alignment == FontAlignment::TOP_LEFT)
      style->Alignment = VALIGN_TOP | HALIGN_LEFT;
    else if (subStyle.alignment == FontAlignment::TOP_CENTER)
      style->Alignment = VALIGN_TOP | HALIGN_CENTER;
    else if (subStyle.alignment == FontAlignment::TOP_RIGHT)
      style->Alignment = VALIGN_TOP | HALIGN_RIGHT;
    else if (subStyle.alignment == FontAlignment::MIDDLE_LEFT)
      style->Alignment = VALIGN_CENTER | HALIGN_LEFT;
    else if (subStyle.alignment == FontAlignment::MIDDLE_CENTER)
      style->Alignment = VALIGN_CENTER | HALIGN_CENTER;
    else if (subStyle.alignment == FontAlignment::MIDDLE_RIGHT)
      style->Alignment = VALIGN_CENTER | HALIGN_RIGHT;
    else if (subStyle.alignment == FontAlignment::SUB_LEFT)
      style->Alignment = VALIGN_SUB | HALIGN_LEFT;
    else if (subStyle.alignment == FontAlignment::SUB_CENTER)
      style->Alignment = VALIGN_SUB | HALIGN_CENTER;
    else if (subStyle.alignment == FontAlignment::SUB_RIGHT)
      style->Alignment = VALIGN_SUB | HALIGN_RIGHT;
  }

  if (m_subtitleType == NATIVE)
  {
    ConfigureAssOverride(subStyle, style);
    m_currentDefaultStyleId = m_track->default_style;
  }
}

void CDVDSubtitlesLibass::ConfigureAssOverride(style& subStyle, ASS_Style* style)
{
  // Default behaviour, disable ASS embedded styles override (if has been changed)
  int stylesFlags = ASS_OVERRIDE_BIT_SELECTIVE_FONT_SCALE;

  if (style)
  {
    // Manage override cases with ASS embedded styles
    if (subStyle.assOverrideStyles == AssOverrideStyles::STYLES)
    {
      stylesFlags = ASS_OVERRIDE_BIT_FONT_SIZE_FIELDS | ASS_OVERRIDE_BIT_FONT_NAME |
                    ASS_OVERRIDE_BIT_COLORS | ASS_OVERRIDE_BIT_ATTRIBUTES |
                    ASS_OVERRIDE_BIT_BORDER | ASS_OVERRIDE_BIT_MARGINS;
    }
    else if (subStyle.assOverrideStyles == AssOverrideStyles::STYLES_POSITIONS)
    {
      stylesFlags = ASS_OVERRIDE_BIT_FONT_SIZE_FIELDS | ASS_OVERRIDE_BIT_FONT_NAME |
                    ASS_OVERRIDE_BIT_COLORS | ASS_OVERRIDE_BIT_ATTRIBUTES |
                    ASS_OVERRIDE_BIT_BORDER | ASS_OVERRIDE_BIT_MARGINS | ASS_OVERRIDE_BIT_ALIGNMENT;
    }
    else if (subStyle.assOverrideStyles == AssOverrideStyles::POSITIONS)
    {
      stylesFlags = ASS_OVERRIDE_BIT_ALIGNMENT;
    }
    ass_set_selective_style_override(m_renderer, style);
  }
  ass_set_selective_style_override_enabled(m_renderer, stylesFlags);
}

void CDVDSubtitlesLibass::ConfigureFont(bool overrideFont, std::string fontName)
{
  int fontProvider = ASS_FONTPROVIDER_AUTODETECT;
  std::string fontPath = GetDefaultFontPath(fontName);
  if ((m_subtitleType == ADAPTED || overrideFont) && !fontPath.empty())
    fontProvider = ASS_FONTPROVIDER_NONE;
  // Libass take in consideration of the default font specified only
  // as last resort so when the builtin list of fallbacks fails,
  // the be able to use our font we have to set ASS_FONTPROVIDER_NONE
  ass_set_fonts(m_renderer, fontPath.c_str(), fontName.c_str(), fontProvider, nullptr, 1);
}

ASS_Event* CDVDSubtitlesLibass::GetEvents()
{
  CSingleLock lock(m_section);
  if (!m_track)
  {
    CLog::Log(LOGERROR, "{} -  Missing ASS structs (m_track)", __FUNCTION__);
    return NULL;
  }
  return m_track->events;
}

int CDVDSubtitlesLibass::GetNrOfEvents() const
{
  CSingleLock lock(m_section);
  if (!m_track)
    return 0;
  return m_track->n_events;
}

int CDVDSubtitlesLibass::AddEvent(const char* text, double startTime, double stopTime)
{
  if (text == NULL || text[0] == '\0')
  {
    CLog::Log(LOGDEBUG,
              "{} - Add event skipped due to empty text (with start time: {}, stop time {})",
              __FUNCTION__, startTime, stopTime);
    return ASS_NO_ID;
  }

  CSingleLock lock(m_section);
  if (!m_library || !m_track)
  {
    CLog::Log(LOGERROR, "{} - Missing ASS structs (m_library or m_track)", __FUNCTION__);
    return ASS_NO_ID;
  }

  int eventId = ass_alloc_event(m_track);
  if (eventId >= 0)
  {
    ASS_Event* event = m_track->events + eventId;
    event->Start = DVD_TIME_TO_MSEC(startTime);
    event->Duration = DVD_TIME_TO_MSEC(stopTime - startTime);
    event->Style = m_defaultKodiStyleId;
    event->ReadOrder = eventId;
    event->Text = strdup(text);
    return eventId;
  }
  else
    CLog::Log(LOGERROR, "{} - Cannot allocate a new event", __FUNCTION__);
  return ASS_NO_ID;
}

void CDVDSubtitlesLibass::AppendTextToEvent(int eventId, const char* text)
{
  CSingleLock lock(m_section);
  if (eventId == ASS_NO_ID || text == NULL || text[0] == '\0')
    return;
  if (!m_track)
  {
    CLog::Log(LOGERROR, "{} -  Missing ASS structs (m_track)", __FUNCTION__);
    return;
  }

  ASS_Event* assEvents = m_track->events;
  if (!assEvents)
  {
    CLog::Log(LOGERROR, "{} -  Failed append text to Event ID {}, there are no Events",
              __FUNCTION__, eventId);
    return;
  }

  ASS_Event* assEvent = (assEvents + eventId);
  if (assEvent)
  {
    int buffSize = strlen(assEvent->Text) + strlen(text) + 1;
    char* appendedText = new char[buffSize];
    strcpy(appendedText, assEvent->Text);
    strcat(appendedText, text);
    free(assEvent->Text);
    assEvent->Text = strdup(appendedText);
    delete[] appendedText;
  }
}

void CDVDSubtitlesLibass::ChangeEventStopTime(int eventId, double stopTime)
{
  CSingleLock lock(m_section);
  if (eventId == ASS_NO_ID)
    return;
  if (!m_track)
  {
    CLog::Log(LOGERROR, "{} -  Missing ASS structs (m_track)", __FUNCTION__);
    return;
  }

  ASS_Event* assEvents = m_track->events;
  if (!assEvents)
  {
    CLog::Log(LOGERROR, "{} -  Failed change stop time to Event ID {}, there are no Events",
              __FUNCTION__, eventId);
    return;
  }

  ASS_Event* assEvent = (assEvents + eventId);
  if (assEvent)
    assEvent->Duration = (DVD_TIME_TO_MSEC(stopTime) - assEvent->Start);
}

void CDVDSubtitlesLibass::FlushEvents()
{
  CSingleLock lock(m_section);
  if (!m_library || !m_track)
  {
    CLog::Log(LOGERROR, "{} - Missing ASS structs (m_library or m_track)", __FUNCTION__);
    return;
  }

  ass_flush_events(m_track);
}

int CDVDSubtitlesLibass::DeleteEvents(int nEvents, int threshold)
{
  CSingleLock lock(m_section);
  if (!m_library || !m_track)
  {
    CLog::Log(LOGERROR, "{} - Missing ASS structs (m_library or m_track)", __FUNCTION__);
    return ASS_NO_ID;
  }

  if (m_track->n_events == 0)
    return ASS_NO_ID;
  if (m_track->n_events < (threshold - nEvents))
    return m_track->n_events - 1;

  // Currently LibAss do not have delete event method we have to free the events
  // and reassign all events starting with the first empty position
  int n = 0;
  for (; n < nEvents; n++)
  {
    ass_free_event(m_track, n);
    m_track->n_events--;
  }
  for (int i = 0; n > 0 && i < threshold; i++)
  {
    m_track->events[i] = m_track->events[i + n];
  }
  return m_track->n_events - 1;
}
