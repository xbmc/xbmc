/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "OverlayRendererGUI.h"

#include "ServiceBroker.h"
#include "Util.h"
#include "cores/VideoPlayer/DVDCodecs/Overlay/DVDOverlayText.h"
#include "filesystem/File.h"
#include "guilib/GUIFont.h"
#include "guilib/GUIFontManager.h"
#include "guilib/GUITextLayout.h"
#include "guilib/GUITexture.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/ColorUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

using namespace OVERLAY;

static UTILS::Color colors[9] = { UTILS::COLOR::YELLOW,
                                  UTILS::COLOR::WHITE,
                                  UTILS::COLOR::BLUE,
                                  UTILS::COLOR::BRIGHTGREEN,
                                  UTILS::COLOR::YELLOWGREEN,
                                  UTILS::COLOR::CYAN,
                                  UTILS::COLOR::LIGHTGREY,
                                  UTILS::COLOR::GREY,
                                  UTILS::COLOR::DARKGREY };

CGUITextLayout* COverlayText::GetFontLayout(const std::string &font, int color, int opacity, int height, int style,
                                            const std::string &fontcache, const std::string &fontbordercache)
{
  if (CUtil::IsUsingTTFSubtitles())
  {
    const std::string& font_file = font;
    std::string font_path = URIUtils::AddFileToFolder("special://home/media/Fonts/", font_file);
    if (!XFILE::CFile::Exists(font_path))
      font_path = URIUtils::AddFileToFolder("special://xbmc/media/Fonts/", font_file);

    // Apply opacity to the color
    UTILS::Color fgcolor = colors[color];
    UTILS::Color bordercolor = UTILS::COLOR::BLACK;
    if (opacity > 0 && opacity < 100)
    {
      fgcolor = ColorUtils::ChangeOpacity(fgcolor, opacity / 100.0f);
      bordercolor = ColorUtils::ChangeOpacity(bordercolor, opacity / 100.0f);
    }
    else if (opacity == 0)
    {
      fgcolor = bordercolor = UTILS::COLOR::NONE;
    }

    // We scale based on PAL4x3 - this at least ensures all sizing is constant across resolutions.
    RESOLUTION_INFO pal(720, 576, 0);
    CGUIFont *subtitle_font = g_fontManager.LoadTTF(fontcache
                                                    , font_path
                                                    , fgcolor
                                                    , 0
                                                    , height
                                                    , style
                                                    , false, 1.0f, 1.0f, &pal, true);
    CGUIFont *border_font   = g_fontManager.LoadTTF(fontbordercache
                                                    , font_path
                                                    , bordercolor
                                                    , 0
                                                    , height
                                                    , style
                                                    , true, 1.0f, 1.0f, &pal, true);
    if (!subtitle_font || !border_font)
      CLog::Log(LOGERROR, "COverlayText::GetFontLayout - Unable to load subtitle font");
    else
      return new CGUITextLayout(subtitle_font, true, 0, border_font);
  }

  return nullptr;
}

COverlayText::COverlayText(CDVDOverlayText * src)
{
  CDVDOverlayText::CElement* e = src->m_pHead;
  while (e)
  {
    if (e->IsElementType(CDVDOverlayText::ELEMENT_TYPE_TEXT))
    {
      CDVDOverlayText::CElementText* t = (CDVDOverlayText::CElementText*)e;
      m_text += t->GetText();
      m_text += "\n";
    }
    e = e->pNext;
  }

  // Avoid additional line breaks
  while(StringUtils::EndsWith(m_text, "\n"))
    m_text = StringUtils::Left(m_text, m_text.length() - 1);

  // Remove HTML-like tags from the subtitles until
  StringUtils::Replace(m_text, "\\r", "");
  StringUtils::Replace(m_text, "\r", "");
  StringUtils::Replace(m_text, "\\n", "[CR]");
  StringUtils::Replace(m_text, "\n", "[CR]");
  StringUtils::Replace(m_text, "<br>", "[CR]");
  StringUtils::Replace(m_text, "\\N", "[CR]");
  StringUtils::Replace(m_text, "<i>", "[I]");
  StringUtils::Replace(m_text, "</i>", "[/I]");
  StringUtils::Replace(m_text, "<b>", "[B]");
  StringUtils::Replace(m_text, "</b>", "[/B]");
  StringUtils::Replace(m_text, "<u>", "");
  StringUtils::Replace(m_text, "<p>", "");
  StringUtils::Replace(m_text, "<P>", "");
  StringUtils::Replace(m_text, "&nbsp;", "");
  StringUtils::Replace(m_text, "</u>", "");
  StringUtils::Replace(m_text, "</i", "[/I]"); // handle tags which aren't closed properly (happens).
  StringUtils::Replace(m_text, "</b", "[/B]");
  StringUtils::Replace(m_text, "</u", "");

  m_subalign = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_SUBTITLES_ALIGN);
  if (m_subalign == SUBTITLE_ALIGN_MANUAL)
  {
    m_align  = ALIGN_SUBTITLE;
    m_pos    = POSITION_RELATIVE;
    m_x      = 0.0f;
    m_y      = 0.0f;
  }
  else
  {
    if(m_subalign == SUBTITLE_ALIGN_TOP_INSIDE ||
       m_subalign == SUBTITLE_ALIGN_BOTTOM_INSIDE)
      m_align  = ALIGN_VIDEO;
    else
      m_align = ALIGN_SCREEN;

    m_pos    = POSITION_RELATIVE;
    m_x      = 0.5f;

    if(m_subalign == SUBTITLE_ALIGN_TOP_INSIDE ||
       m_subalign == SUBTITLE_ALIGN_TOP_OUTSIDE)
      m_y    = 0.0f;
    else
      m_y    = 1.0f;
  }
  m_width  = 0;
  m_height = 0;

  m_type = TYPE_GUITEXT;
  m_layout = nullptr;
}

COverlayText::~COverlayText()
{
  delete m_layout;
}

void COverlayText::PrepareRender(const std::string &font, int color, int opacity, int height, int style, const std::string &fontcache,
                                 const std::string &fontbordercache, const UTILS::Color bgcolor, const CRect &rectView)
{
  if (!m_layout)
    m_layout = GetFontLayout(font, color, opacity, height, style, fontcache, fontbordercache);

  if (m_layout == nullptr)
  {
    CLog::Log(LOGERROR, "COverlayText::PrepareRender - GetFontLayout failed for font %s", font.c_str());
    return;
  }
  
  m_rv = rectView;
  m_bgcolor = bgcolor;
  m_bordercolor = ColorUtils::ChangeOpacity(UTILS::COLOR::BLACK, opacity / 100.0f);
  
  m_layout->Update(m_text, m_rv.Width() * 0.9f, false, true); // true to force LTR reading order (most Hebrew subs are this format)
  m_layout->GetTextExtent(m_width, m_height);
  
}

void COverlayText::Render(OVERLAY::SRenderState &state)
{
  if(m_layout == nullptr)
    return;

  float x = state.x;
  float y = state.y;

  if (m_subalign == SUBTITLE_ALIGN_MANUAL
  ||  m_subalign == SUBTITLE_ALIGN_BOTTOM_OUTSIDE
  ||  m_subalign == SUBTITLE_ALIGN_BOTTOM_INSIDE)
    y -= m_height;
  
  // draw the overlay background
  if (m_bgcolor != UTILS::COLOR::NONE)
  {
    CRect backgroundbox(x - m_layout->GetTextWidth() * 0.52f, y, x + m_layout->GetTextWidth() * 0.52f, y + m_layout->GetTextHeight());
    CGUITexture::DrawQuad(backgroundbox, m_bgcolor);
  }

  m_layout->RenderOutline(x, y, 0, m_bordercolor, XBFONT_CENTER_X, m_rv.Width());
  CServiceBroker::GetWinSystem()->GetGfxContext().RemoveTransform();
}
