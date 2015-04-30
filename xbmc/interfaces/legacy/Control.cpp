/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "Control.h"
#include "LanguageHook.h"
#include "AddonUtils.h"

#include "guilib/GUILabel.h"
#include "guilib/GUIFontManager.h"
#include "guilib/GUILabelControl.h"
#include "guilib/GUIFadeLabelControl.h"
#include "guilib/GUITextBox.h"
#include "guilib/GUIButtonControl.h"
#include "guilib/GUICheckMarkControl.h"
#include "guilib/GUIImage.h"
#include "guilib/GUIListContainer.h"
#include "guilib/GUIProgressControl.h"
#include "guilib/GUISliderControl.h"
#include "guilib/GUIRadioButtonControl.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/GUIEditControl.h"
#include "guilib/GUIControlFactory.h"
#include "listproviders/StaticProvider.h"

#include "utils/XBMCTinyXML.h"
#include "utils/StringUtils.h"
#include "WindowException.h"

namespace XBMCAddon
{
  namespace xbmcgui
  {

    // ============================================================

    // ============================================================
    // ============================================================
    ControlFadeLabel::ControlFadeLabel(long x, long y, long width, long height,
                                       const char* font, const char* _textColor,
                                       long _alignment) :
      strFont("font13"), textColor(0xffffffff), align(_alignment)
    {
      dwPosX = x;
      dwPosY = y;
      dwWidth = width;
      dwHeight = height;

      if (font)
        strFont = font;

      if (_textColor)
        sscanf(_textColor, "%x", &textColor);

      pGUIControl = NULL;
    }

    void ControlFadeLabel::addLabel(const String& label)
    {
      CGUIMessage msg(GUI_MSG_LABEL_ADD, iParentId, iControlId);
      msg.SetLabel(label);

      g_windowManager.SendThreadMessage(msg, iParentId);
    }

    void ControlFadeLabel::reset()
    {
      CGUIMessage msg(GUI_MSG_LABEL_RESET, iParentId, iControlId);

      vecLabels.clear();
      g_windowManager.SendThreadMessage(msg, iParentId);
    }

    CGUIControl* ControlFadeLabel::Create()
    {
      CLabelInfo label;
      label.font = g_fontManager.GetFont(strFont);
      label.textColor = label.focusedColor = textColor;
      label.align = align;
      pGUIControl = new CGUIFadeLabelControl(
        iParentId,
        iControlId,
        (float)dwPosX,
        (float)dwPosY,
        (float)dwWidth,
        (float)dwHeight,
        label,
        true,
        0,
        true);

      CGUIMessage msg(GUI_MSG_LABEL_RESET, iParentId, iControlId);
      pGUIControl->OnMessage(msg);

      return pGUIControl;
    }

    // ============================================================

    // ============================================================
    // ============================================================
    ControlTextBox::ControlTextBox(long x, long y, long width, long height,
                                   const char* font, const char* _textColor) :
      strFont("font13"), textColor(0xffffffff)
    {
      dwPosX = x;
      dwPosY = y;
      dwWidth = width;
      dwHeight = height;

      if (font)
        strFont = font;

      if (_textColor)
        sscanf(_textColor, "%x", &textColor);
    }

    void ControlTextBox::setText(const String& text)
    {
      // create message
      CGUIMessage msg(GUI_MSG_LABEL_SET, iParentId, iControlId);
      msg.SetLabel(text);

      // send message
      g_windowManager.SendThreadMessage(msg, iParentId);
    }

    String ControlTextBox::getText()
    {
      if (!pGUIControl) return NULL;

      LOCKGUI;
      return ((CGUITextBox*) pGUIControl)->GetDescription();
    }

    void ControlTextBox::reset()
    {
      // create message
      CGUIMessage msg(GUI_MSG_LABEL_RESET, iParentId, iControlId);
      g_windowManager.SendThreadMessage(msg, iParentId);
    }

    void ControlTextBox::scroll(long position)
    {
      static_cast<CGUITextBox*>(pGUIControl)->Scroll((int)position);
    }

    void ControlTextBox::autoScroll(int delay, int time, int repeat)
    {
      static_cast<CGUITextBox*>(pGUIControl)->SetAutoScrolling(delay, time, repeat);
    }

    CGUIControl* ControlTextBox::Create()
    {
      // create textbox
      CLabelInfo label;
      label.font = g_fontManager.GetFont(strFont);
      label.textColor = label.focusedColor = textColor;

      pGUIControl = new CGUITextBox(iParentId, iControlId,
           (float)dwPosX, (float)dwPosY, (float)dwWidth, (float)dwHeight,
           label);

      // reset textbox
      CGUIMessage msg(GUI_MSG_LABEL_RESET, iParentId, iControlId);
      pGUIControl->OnMessage(msg);

      return pGUIControl;
    }

    // ============================================================

    // ============================================================
    // ============================================================
    ControlButton::ControlButton(long x, long y, long width, long height, const String& label,
                                 const char* focusTexture, const char* noFocusTexture,
                                 long _textOffsetX, long _textOffsetY,
                                 long alignment, const char* font, const char* _textColor,
                                 const char* _disabledColor, long angle,
                                 const char* _shadowColor, const char* _focusedColor) :
      textOffsetX(_textOffsetX), textOffsetY(_textOffsetY),
      align(alignment), strFont("font13"), textColor(0xffffffff), disabledColor(0x60ffffff),
      iAngle(angle), shadowColor(0), focusedColor(0xffffffff)
    {
      dwPosX = x;
      dwPosY = y;
      dwWidth = width;
      dwHeight = height;

      strText = label;

      // if texture is supplied use it, else get default ones
      strTextureFocus = focusTexture ? focusTexture :
        XBMCAddonUtils::getDefaultImage((char*)"button", (char*)"texturefocus", (char*)"button-focus.png");
      strTextureNoFocus = noFocusTexture ? noFocusTexture :
        XBMCAddonUtils::getDefaultImage((char*)"button", (char*)"texturenofocus", (char*)"button-nofocus.jpg");

      if (font) strFont = font;
      if (_textColor) sscanf( _textColor, "%x", &textColor );
      if (_disabledColor) sscanf( _disabledColor, "%x", &disabledColor );
      if (_shadowColor) sscanf( _shadowColor, "%x", &shadowColor );
      if (_focusedColor) sscanf( _focusedColor, "%x", &focusedColor );
    }

    void ControlButton::setLabel(const String& label,
                                 const char* font,
                                 const char* _textColor,
                                 const char* _disabledColor,
                                 const char* _shadowColor,
                                 const char* _focusedColor,
                                 const String& label2)
    {
      if (!label.empty()) strText = label;
      if (!label2.empty()) strText2 = label2;
      if (font) strFont = font;
      if (_textColor) sscanf(_textColor, "%x", &textColor);
      if (_disabledColor) sscanf( _disabledColor, "%x", &disabledColor );
      if (_shadowColor) sscanf(_shadowColor, "%x", &shadowColor);
      if (_focusedColor) sscanf(_focusedColor, "%x", &focusedColor);

      if (pGUIControl)
      {
        LOCKGUI;
        ((CGUIButtonControl*)pGUIControl)->PythonSetLabel(strFont, strText, textColor, shadowColor, focusedColor);
        ((CGUIButtonControl*)pGUIControl)->SetLabel2(strText2);
        ((CGUIButtonControl*)pGUIControl)->PythonSetDisabledColor(disabledColor);
      }
    }

    void ControlButton::setDisabledColor(const char* color)
    {
      if (color) sscanf(color, "%x", &disabledColor);

      if (pGUIControl)
      {
        LOCKGUI;
        ((CGUIButtonControl*)pGUIControl)->PythonSetDisabledColor(disabledColor);
      }
    }

    String ControlButton::getLabel()
    {
      if (!pGUIControl) return NULL;

      LOCKGUI;
      return ((CGUIButtonControl*) pGUIControl)->GetLabel();
    }

    String ControlButton::getLabel2()
    {
      if (!pGUIControl) return NULL;

      LOCKGUI;
      return ((CGUIButtonControl*) pGUIControl)->GetLabel2();
    }

    CGUIControl* ControlButton::Create()
    {
      CLabelInfo label;
      label.font = g_fontManager.GetFont(strFont);
      label.textColor = textColor;
      label.disabledColor = disabledColor;
      label.shadowColor = shadowColor;
      label.focusedColor = focusedColor;
      label.align = align;
      label.offsetX = (float)textOffsetX;
      label.offsetY = (float)textOffsetY;
      label.angle = (float)-iAngle;
      pGUIControl = new CGUIButtonControl(
        iParentId,
        iControlId,
        (float)dwPosX,
        (float)dwPosY,
        (float)dwWidth,
        (float)dwHeight,
        CTextureInfo(strTextureFocus),
        CTextureInfo(strTextureNoFocus),
        label);

      CGUIButtonControl* pGuiButtonControl =
        (CGUIButtonControl*)pGUIControl;

      pGuiButtonControl->SetLabel(strText);
      pGuiButtonControl->SetLabel2(strText2);

      return pGUIControl;
    }

    // ============================================================

    // ============================================================
    // ============================================================
    ControlCheckMark::ControlCheckMark(long x, long y, long width, long height, const String& label,
                                       const char* focusTexture, const char* noFocusTexture,
                                       long _checkWidth, long _checkHeight,
                                       long _alignment, const char* font,
                                       const char* _textColor, const char* _disabledColor) :
      strFont("font13"), checkWidth(_checkWidth), checkHeight(_checkHeight),
      align(_alignment), textColor(0xffffffff), disabledColor(0x60ffffff)
    {
      dwPosX = x;
      dwPosY = y;
      dwWidth = width;
      dwHeight = height;

      strText = label;
      if (font) strFont = font;
      if (_textColor) sscanf(_textColor, "%x", &textColor);
      if (_disabledColor) sscanf( _disabledColor, "%x", &disabledColor );

      strTextureFocus = focusTexture ?  focusTexture :
        XBMCAddonUtils::getDefaultImage((char*)"checkmark", (char*)"texturefocus", (char*)"check-box.png");
      strTextureNoFocus = noFocusTexture ? noFocusTexture :
        XBMCAddonUtils::getDefaultImage((char*)"checkmark", (char*)"texturenofocus", (char*)"check-boxNF.png");
    }

    bool ControlCheckMark::getSelected()
    {
      bool isSelected = false;

      if (pGUIControl)
      {
        LOCKGUI;
        isSelected = ((CGUICheckMarkControl*)pGUIControl)->GetSelected();
      }

      return isSelected;
    }

    void ControlCheckMark::setSelected(bool selected)
    {
      if (pGUIControl)
      {
        LOCKGUI;
        ((CGUICheckMarkControl*)pGUIControl)->SetSelected(selected);
      }
    }

    void ControlCheckMark::setLabel(const String& label,
                                    const char* font,
                                    const char* _textColor,
                                    const char* _disabledColor,
                                    const char* _shadowColor,
                                    const char* _focusedColor,
                                    const String& label2)
    {

      if (font) strFont = font;
      if (_textColor) sscanf(_textColor, "%x", &textColor);
      if (_disabledColor) sscanf(_disabledColor, "%x", &disabledColor);

      if (pGUIControl)
      {
        LOCKGUI;
        ((CGUICheckMarkControl*)pGUIControl)->PythonSetLabel(strFont,strText,textColor);
        ((CGUICheckMarkControl*)pGUIControl)->PythonSetDisabledColor(disabledColor);
      }
    }

    void ControlCheckMark::setDisabledColor(const char* color)
    {
      if (color) sscanf(color, "%x", &disabledColor);

      if (pGUIControl)
      {
        LOCKGUI;
        ((CGUICheckMarkControl*)pGUIControl)->PythonSetDisabledColor( disabledColor );
      }
    }

    CGUIControl* ControlCheckMark::Create()
    {
      CLabelInfo label;
      label.disabledColor = disabledColor;
      label.textColor = label.focusedColor = textColor;
      label.font = g_fontManager.GetFont(strFont);
      label.align = align;
      CTextureInfo imageFocus(strTextureFocus);
      CTextureInfo imageNoFocus(strTextureNoFocus);
      pGUIControl = new CGUICheckMarkControl(
        iParentId,
        iControlId,
        (float)dwPosX,
        (float)dwPosY,
        (float)dwWidth,
        (float)dwHeight,
        imageFocus, imageNoFocus,
        (float)checkWidth,
        (float)checkHeight,
        label );

      CGUICheckMarkControl* pGuiCheckMarkControl = (CGUICheckMarkControl*)pGUIControl;
      pGuiCheckMarkControl->SetLabel(strText);

      return pGUIControl;
    }

    // ============================================================

    // ============================================================
    // ============================================================
    ControlImage::ControlImage(long x, long y, long width, long height,
                               const char* filename, long aRatio,
                               const char* _colorDiffuse):
      aspectRatio(aRatio), colorDiffuse(0)
    {
      dwPosX = x;
      dwPosY = y;
      dwWidth = width;
      dwHeight = height;

      // check if filename exists
      strFileName = filename;
      if (_colorDiffuse)
        sscanf(_colorDiffuse, "%x", &colorDiffuse);
    }

    void ControlImage::setImage(const char* imageFilename, const bool useCache)
    {
      strFileName = imageFilename;

      LOCKGUI;
      if (pGUIControl)
        ((CGUIImage*)pGUIControl)->SetFileName(strFileName, false, useCache);
    }

    void ControlImage::setColorDiffuse(const char* cColorDiffuse)
    {
      if (cColorDiffuse) sscanf(cColorDiffuse, "%x", &colorDiffuse);
      else colorDiffuse = 0;

      LOCKGUI;
      if (pGUIControl)
        ((CGUIImage *)pGUIControl)->SetColorDiffuse(colorDiffuse);
    }

    CGUIControl* ControlImage::Create()
    {
      pGUIControl = new CGUIImage(iParentId, iControlId,
            (float)dwPosX, (float)dwPosY, (float)dwWidth, (float)dwHeight,
            CTextureInfo(strFileName));

      if (pGUIControl && aspectRatio <= CAspectRatio::AR_KEEP)
        ((CGUIImage *)pGUIControl)->SetAspectRatio((CAspectRatio::ASPECT_RATIO)aspectRatio);

      if (pGUIControl && colorDiffuse)
        ((CGUIImage *)pGUIControl)->SetColorDiffuse(colorDiffuse);

      return pGUIControl;
    }

    // ============================================================

    // ============================================================
    // ============================================================
    ControlProgress::ControlProgress(long x, long y, long width, long height,
                                     const char* texturebg,
                                     const char* textureleft,
                                     const char* texturemid,
                                     const char* textureright,
                                     const char* textureoverlay)
    {
      dwPosX = x;
      dwPosY = y;
      dwWidth = width;
      dwHeight = height;

      // if texture is supplied use it, else get default ones
      strTextureBg = texturebg ? texturebg :
        XBMCAddonUtils::getDefaultImage((char*)"progress", (char*)"texturebg", (char*)"progress_back.png");
      strTextureLeft = textureleft ? textureleft :
        XBMCAddonUtils::getDefaultImage((char*)"progress", (char*)"lefttexture", (char*)"progress_left.png");
      strTextureMid = texturemid ? texturemid :
        XBMCAddonUtils::getDefaultImage((char*)"progress", (char*)"midtexture", (char*)"progress_mid.png");
      strTextureRight = textureright ? textureright :
        XBMCAddonUtils::getDefaultImage((char*)"progress", (char*)"righttexture", (char*)"progress_right.png");
      strTextureOverlay = textureoverlay ? textureoverlay :
        XBMCAddonUtils::getDefaultImage((char*)"progress", (char*)"overlaytexture", (char*)"progress_over.png");
    }

    void ControlProgress::setPercent(float pct)
    {
      if (pGUIControl)
        ((CGUIProgressControl*)pGUIControl)->SetPercentage(pct);
    }

    float ControlProgress::getPercent()
    {
      return (pGUIControl) ? ((CGUIProgressControl*)pGUIControl)->GetPercentage() : 0.0f;
    }

    CGUIControl* ControlProgress::Create()
    {
      pGUIControl = new CGUIProgressControl(iParentId, iControlId,
         (float)dwPosX, (float)dwPosY,
         (float)dwWidth,(float)dwHeight,
         CTextureInfo(strTextureBg), CTextureInfo(strTextureLeft),
         CTextureInfo(strTextureMid), CTextureInfo(strTextureRight),
         CTextureInfo(strTextureOverlay));

      if (pGUIControl && colorDiffuse)
        ((CGUIProgressControl *)pGUIControl)->SetColorDiffuse(colorDiffuse);

      return pGUIControl;
    }

    // ============================================================

    // ============================================================
    // ============================================================
    ControlSlider::ControlSlider(long x, long y, long width, long height,
                                 const char* textureback,
                                 const char* texture,
                                 const char* texturefocus)
    {
      dwPosX = x;
      dwPosY = y;
      dwWidth = width;
      dwHeight = height;

      // if texture is supplied use it, else get default ones
      strTextureBack = textureback ? textureback :
        XBMCAddonUtils::getDefaultImage((char*)"slider", (char*)"texturesliderbar", (char*)"osd_slider_bg_2.png");
      strTexture = texture ? texture :
        XBMCAddonUtils::getDefaultImage((char*)"slider", (char*)"textureslidernib", (char*)"osd_slider_nibNF.png");
      strTextureFoc = texturefocus ? texturefocus :
        XBMCAddonUtils::getDefaultImage((char*)"slider", (char*)"textureslidernibfocus", (char*)"osd_slider_nib.png");
    }

    float ControlSlider::getPercent()
    {
      return (pGUIControl) ? ((CGUISliderControl*)pGUIControl)->GetPercentage() : 0.0f;
    }

    void ControlSlider::setPercent(float pct)
    {
      if (pGUIControl)
        ((CGUISliderControl*)pGUIControl)->SetPercentage(pct);
    }

    CGUIControl* ControlSlider::Create ()
    {
      pGUIControl = new CGUISliderControl(iParentId, iControlId,(float)dwPosX, (float)dwPosY,
                                          (float)dwWidth,(float)dwHeight,
                                          CTextureInfo(strTextureBack),CTextureInfo(strTexture),
                                          CTextureInfo(strTextureFoc),0);

      return pGUIControl;
    }

    // ============================================================

    // ============================================================
    // ============================================================
    ControlGroup::ControlGroup(long x, long y, long width, long height)
    {
      dwPosX = x;
      dwPosY = y;
      dwWidth = width;
      dwHeight = height;
    }

    CGUIControl* ControlGroup::Create()
    {
      pGUIControl = new CGUIControlGroup(iParentId,
                                         iControlId,
                                         (float) dwPosX,
                                         (float) dwPosY,
                                         (float) dwWidth,
                                         (float) dwHeight);
      return pGUIControl;
    }

    // ============================================================

    // ============================================================
    // ============================================================
    ControlRadioButton::ControlRadioButton(long x, long y, long width, long height, const String& label,
                                           const char* focusOnTexture,  const char* noFocusOnTexture,
                                           const char* focusOffTexture, const char* noFocusOffTexture,
                                           const char* focusTexture,    const char* noFocusTexture,
                                           long _textOffsetX, long _textOffsetY,
                                           long alignment, const char* font, const char* _textColor,
                                           const char* _disabledColor, long angle,
                                           const char* _shadowColor, const char* _focusedColor) :
      strFont("font13"), textColor(0xffffffff), disabledColor(0x60ffffff),
      textOffsetX(_textOffsetX), textOffsetY(_textOffsetY), align(alignment), iAngle(angle),
      shadowColor(0), focusedColor(0xffffffff)
    {
      dwPosX = x;
      dwPosY = y;
      dwWidth = width;
      dwHeight = height;

      strText = label;

      // if texture is supplied use it, else get default ones
      strTextureFocus = focusTexture ? focusTexture :
        XBMCAddonUtils::getDefaultImage((char*)"button", (char*)"texturefocus", (char*)"button-focus.png");
      strTextureNoFocus = noFocusTexture ? noFocusTexture :
        XBMCAddonUtils::getDefaultImage((char*)"button", (char*)"texturenofocus", (char*)"button-nofocus.jpg");

      if (focusOnTexture && noFocusOnTexture)
      {
        strTextureRadioOnFocus = focusOnTexture;
        strTextureRadioOnNoFocus = noFocusOnTexture;
      }
      else
      {
        strTextureRadioOnFocus = strTextureRadioOnNoFocus = focusTexture ? focusTexture :
          XBMCAddonUtils::getDefaultImage((char*)"radiobutton", (char*)"textureradiofocus", (char*)"radiobutton-focus.png");
      }

      if (focusOffTexture && noFocusOffTexture)
      {
        strTextureRadioOffFocus = focusOffTexture;
        strTextureRadioOffNoFocus = noFocusOffTexture;
      }
      else
      {
        strTextureRadioOffFocus = strTextureRadioOffNoFocus = noFocusTexture ? noFocusTexture :
          XBMCAddonUtils::getDefaultImage((char*)"radiobutton", (char*)"textureradiofocus", (char*)"radiobutton-focus.png");
      }

      if (font) strFont = font;
      if (_textColor) sscanf( _textColor, "%x", &textColor );
      if (_disabledColor) sscanf( _disabledColor, "%x", &disabledColor );
      if (_shadowColor) sscanf( _shadowColor, "%x", &shadowColor );
      if (_focusedColor) sscanf( _focusedColor, "%x", &focusedColor );
    }

    void ControlRadioButton::setSelected(bool selected)
    {
      if (pGUIControl)
      {
        LOCKGUI;
        ((CGUIRadioButtonControl*)pGUIControl)->SetSelected(selected);
      }
    }

    bool ControlRadioButton::isSelected()
    {
      bool isSelected = false;

      if (pGUIControl)
      {
        LOCKGUI;
        isSelected = ((CGUIRadioButtonControl*)pGUIControl)->IsSelected();
      }
      return isSelected;
    }

    void ControlRadioButton::setLabel(const String& label,
                                      const char* font,
                                      const char* _textColor,
                                      const char* _disabledColor,
                                      const char* _shadowColor,
                                      const char* _focusedColor,
                                      const String& label2)
    {
      if (!label.empty()) strText = label;
      if (font) strFont = font;
      if (_textColor) sscanf(_textColor, "%x", &textColor);
      if (_disabledColor) sscanf( _disabledColor, "%x", &disabledColor );
      if (_shadowColor) sscanf(_shadowColor, "%x", &shadowColor);
      if (_focusedColor) sscanf(_focusedColor, "%x", &focusedColor);

      if (pGUIControl)
      {
        LOCKGUI;
        ((CGUIRadioButtonControl*)pGUIControl)->PythonSetLabel(strFont, strText, textColor, shadowColor, focusedColor);
        ((CGUIRadioButtonControl*)pGUIControl)->PythonSetDisabledColor(disabledColor);
      }
    }

    void ControlRadioButton::setRadioDimension(long x, long y, long width, long height)
    {
      dwPosX = x;
      dwPosY = y;
      dwWidth = width;
      dwHeight = height;
      if (pGUIControl)
      {
        LOCKGUI;
        ((CGUIRadioButtonControl*)pGUIControl)->SetRadioDimensions((float)dwPosX, (float)dwPosY, (float)dwWidth, (float)dwHeight);
      }
    }

    CGUIControl* ControlRadioButton::Create()
    {
      CLabelInfo label;
      label.font = g_fontManager.GetFont(strFont);
      label.textColor = textColor;
      label.disabledColor = disabledColor;
      label.shadowColor = shadowColor;
      label.focusedColor = focusedColor;
      label.align = align;
      label.offsetX = (float)textOffsetX;
      label.offsetY = (float)textOffsetY;
      label.angle = (float)-iAngle;
      pGUIControl = new CGUIRadioButtonControl(
        iParentId,
        iControlId,
        (float)dwPosX,
        (float)dwPosY,
        (float)dwWidth,
        (float)dwHeight,
        CTextureInfo(strTextureFocus),
        CTextureInfo(strTextureNoFocus),
        label,
        CTextureInfo(strTextureRadioOnFocus),
        CTextureInfo(strTextureRadioOnNoFocus),
        CTextureInfo(strTextureRadioOffFocus),
        CTextureInfo(strTextureRadioOffNoFocus));

      CGUIRadioButtonControl* pGuiButtonControl =
        (CGUIRadioButtonControl*)pGUIControl;

      pGuiButtonControl->SetLabel(strText);

      return pGUIControl;
    }

    // ============================================================

    // ============================================================
    // ============================================================
    Control::~Control() { deallocating(); }

    CGUIControl* Control::Create()
    {
      throw WindowException("Object is a Control, but can't be added to a window");
    }

    std::vector<int> Control::getPosition()
    {
      std::vector<int> ret(2);
      ret[0] = dwPosX;
      ret[1] = dwPosY;
      return ret;
    }

    void Control::setEnabled(bool enabled)
    {
      DelayedCallGuard dcguard(languageHook);
      LOCKGUI;
      if (pGUIControl)
        pGUIControl->SetEnabled(enabled);
    }

    void Control::setVisible(bool visible)
    {
      DelayedCallGuard dcguard(languageHook);
      LOCKGUI;
      if (pGUIControl)
        pGUIControl->SetVisible(visible);
    }

    void Control::setVisibleCondition(const char* visible, bool allowHiddenFocus)
    {
      DelayedCallGuard dcguard(languageHook);
      LOCKGUI;

      if (pGUIControl)
        pGUIControl->SetVisibleCondition(visible, allowHiddenFocus ? "true" : "false");
    }

    void Control::setEnableCondition(const char* enable)
    {
      DelayedCallGuard dcguard(languageHook);
      LOCKGUI;

      if (pGUIControl)
        pGUIControl->SetEnableCondition(enable);
    }

    void Control::setAnimations(const std::vector< Tuple<String,String> >& eventAttr)
    {
      CXBMCTinyXML xmlDoc;
      TiXmlElement xmlRootElement("control");
      TiXmlNode *pRoot = xmlDoc.InsertEndChild(xmlRootElement);
      if (!pRoot)
        throw WindowException("TiXmlNode creation error");

      std::vector<CAnimation> animations;

      for (unsigned int anim = 0; anim < eventAttr.size(); anim++)
      {
        const Tuple<String,String>& pTuple = eventAttr[anim];

        if (pTuple.GetNumValuesSet() != 2)
          throw WindowException("Error unpacking tuple found in list");

        const String& cEvent = pTuple.first();
        const String& cAttr = pTuple.second();

        TiXmlElement pNode("animation");
        std::vector<std::string> attrs = StringUtils::Split(cAttr, " ");
        for (std::vector<std::string>::const_iterator i = attrs.begin(); i != attrs.end(); ++i)
        {
          std::vector<std::string> attrs2 = StringUtils::Split(*i, "=");
          if (attrs2.size() == 2)
            pNode.SetAttribute(attrs2[0], attrs2[1]);
        }
        TiXmlText value(cEvent.c_str());
        pNode.InsertEndChild(value);
        pRoot->InsertEndChild(pNode);
      }

      const CRect animRect((float)dwPosX, (float)dwPosY, (float)dwPosX + dwWidth, (float)dwPosY + dwHeight);
      LOCKGUI;
      if (pGUIControl)
      {
        CGUIControlFactory::GetAnimations(pRoot, animRect, iParentId, animations);
        pGUIControl->SetAnimations(animations);
      }
    }

    void Control::setPosition(long x, long y)
    {
      DelayedCallGuard dcguard(languageHook);
      LOCKGUI;
      dwPosX = x;
      dwPosY = y;
      if (pGUIControl)
        pGUIControl->SetPosition((float)dwPosX, (float)dwPosY);
    }

    void Control::setWidth(long width)
    {
      DelayedCallGuard dcguard(languageHook);
      LOCKGUI;
      dwWidth = width;
      if (pGUIControl)
        pGUIControl->SetWidth((float)dwWidth);
    }

    void Control::setHeight(long height)
    {
      DelayedCallGuard dcguard(languageHook);
      LOCKGUI;
      dwHeight = height;
      if (pGUIControl)
        pGUIControl->SetHeight((float)dwHeight);
    }

    void Control::setNavigation(const Control* up, const Control* down,
                                const Control* left, const Control* right)
    {
      if(iControlId == 0)
        throw WindowException("Control has to be added to a window first");

      {
        LOCKGUI;
        if (pGUIControl)
        {
          pGUIControl->SetNavigationAction(ACTION_MOVE_UP,    up->iControlId);
          pGUIControl->SetNavigationAction(ACTION_MOVE_DOWN,  down->iControlId);
          pGUIControl->SetNavigationAction(ACTION_MOVE_LEFT,  left->iControlId);
          pGUIControl->SetNavigationAction(ACTION_MOVE_RIGHT, right->iControlId);
        }
      }
    }

    void Control::controlUp(const Control* control)
    {
      if(iControlId == 0)
        throw WindowException("Control has to be added to a window first");

      {
        LOCKGUI;
        if (pGUIControl)
          pGUIControl->SetNavigationAction(ACTION_MOVE_UP, control->iControlId);
      }
    }

    void Control::controlDown(const Control* control)
    {
      if(iControlId == 0)
        throw WindowException("Control has to be added to a window first");

      {
        LOCKGUI;
        if (pGUIControl)
          pGUIControl->SetNavigationAction(ACTION_MOVE_DOWN, control->iControlId);
      }
    }

    void Control::controlLeft(const Control* control)
    {
      if(iControlId == 0)
        throw WindowException("Control has to be added to a window first");

      {
        LOCKGUI;
        if (pGUIControl)
          pGUIControl->SetNavigationAction(ACTION_MOVE_LEFT, control->iControlId);
      }
    }

    void Control::controlRight(const Control* control)
    {
      if(iControlId == 0)
        throw WindowException("Control has to be added to a window first");

      {
        LOCKGUI;
        if (pGUIControl)
          pGUIControl->SetNavigationAction(ACTION_MOVE_RIGHT, control->iControlId);
      }
    }

    // ============================================================
    //  ControlSpin
    // ============================================================
    ControlSpin::ControlSpin()
    {
      // default values for spin control
      color = 0xffffffff;
      dwPosX = 0;
      dwPosY = 0;
      dwWidth = 16;
      dwHeight = 16;

      // get default images
      strTextureUp = XBMCAddonUtils::getDefaultImage((char*)"listcontrol", (char*)"textureup", (char*)"scroll-up.png");
      strTextureDown = XBMCAddonUtils::getDefaultImage((char*)"listcontrol", (char*)"texturedown", (char*)"scroll-down.png");
      strTextureUpFocus = XBMCAddonUtils::getDefaultImage((char*)"listcontrol", (char*)"textureupfocus", (char*)"scroll-up-focus.png");
      strTextureDownFocus = XBMCAddonUtils::getDefaultImage((char*)"listcontrol", (char*)"texturedownfocus", (char*)"scroll-down-focus.png");
    }

    void ControlSpin::setTextures(const char* up, const char* down,
                                  const char* upFocus,
                                  const char* downFocus)
    {
      strTextureUp = up;
      strTextureDown = down;
      strTextureUpFocus = upFocus;
      strTextureDownFocus = downFocus;
      /*
        PyXBMCGUILock();
        if (self->pGUIControl)
        {
        CGUISpinControl* pControl = (CGUISpinControl*)self->pGUIControl;
        pControl->se
        PyXBMCGUIUnlock();
      */
    }

    ControlSpin::~ControlSpin() {}
    // ============================================================

    // ============================================================
    //  ControlLabel
    // ============================================================
    ControlLabel::ControlLabel(long x, long y, long width, long height,
                               const String& label,
                               const char* font, const char* p_textColor,
                               const char* p_disabledColor,
                               long p_alignment,
                               bool hasPath, long angle) :
      strFont("font13"),
      textColor(0xffffffff), disabledColor(0x60ffffff),
      align(p_alignment), bHasPath(hasPath), iAngle(angle)
    {
      dwPosX = x;
      dwPosY = y;
      dwWidth = width;
      dwHeight = height;

      strText = label;
      if (font)
        strFont = font;

      if (p_textColor)
        sscanf(p_textColor, "%x", &textColor);

      if (p_disabledColor)
        sscanf( p_disabledColor, "%x", &disabledColor );
    }

    ControlLabel::~ControlLabel() {}

    CGUIControl* ControlLabel::Create()
    {
      CLabelInfo label;
      label.font = g_fontManager.GetFont(strFont);
      label.textColor = label.focusedColor = textColor;
      label.disabledColor = disabledColor;
      label.align = align;
      label.angle = (float)-iAngle;
      pGUIControl = new CGUILabelControl(
        iParentId,
        iControlId,
        (float)dwPosX,
        (float)dwPosY,
        (float)dwWidth,
        (float)dwHeight,
        label,
        false,
        bHasPath);
      ((CGUILabelControl *)pGUIControl)->SetLabel(strText);
      return pGUIControl;
    }

    void ControlLabel::setLabel(const String& label, const char* font,
                                const char* textColor, const char* disabledColor,
                                const char* shadowColor, const char* focusedColor,
                                const String& label2)
    {
      strText = label;
      CGUIMessage msg(GUI_MSG_LABEL_SET, iParentId, iControlId);
      msg.SetLabel(strText);
      g_windowManager.SendThreadMessage(msg, iParentId);
    }

    String ControlLabel::getLabel()
    {
      if (!pGUIControl)
        return NULL;
      return strText;
    }
    // ============================================================

    // ============================================================
    //  ControlEdit
    // ============================================================
    ControlEdit::ControlEdit(long x, long y, long width, long height, const String& label,
                             const char* font, const char* _textColor,
                             const char* _disabledColor,
                             long _alignment, const char* focusTexture,
                             const char* noFocusTexture, bool isPassword) :
      strFont("font13"), textColor(0xffffffff), disabledColor(0x60ffffff),
      align(_alignment), bIsPassword(isPassword)

    {
      strTextureFocus = focusTexture ? focusTexture :
        XBMCAddonUtils::getDefaultImage((char*)"edit", (char*)"texturefocus", (char*)"button-focus.png");

      strTextureNoFocus = noFocusTexture ? noFocusTexture :
        XBMCAddonUtils::getDefaultImage((char*)"edit", (char*)"texturenofocus", (char*)"button-focus.png");

      if (font) strFont = font;
      if (_textColor) sscanf( _textColor, "%x", &textColor );
      if (_disabledColor) sscanf( _disabledColor, "%x", &disabledColor );
    }

    CGUIControl* ControlEdit::Create()
    {
      CLabelInfo label;
      label.font = g_fontManager.GetFont(strFont);
      label.textColor = label.focusedColor = textColor;
      label.disabledColor = disabledColor;
      label.align = align;
      pGUIControl = new CGUIEditControl(
        iParentId,
        iControlId,
        (float)dwPosX,
        (float)dwPosY,
        (float)dwWidth,
        (float)dwHeight,
        CTextureInfo(strTextureFocus),
        CTextureInfo(strTextureNoFocus),
        label,
        strText);

      if (bIsPassword)
        ((CGUIEditControl *) pGUIControl)->SetInputType(CGUIEditControl::INPUT_TYPE_PASSWORD, 0);
      return pGUIControl;
    }

    void ControlEdit::setLabel(const String& label, const char* font,
                                const char* textColor, const char* disabledColor,
                                const char* shadowColor, const char* focusedColor,
                                const String& label2)
    {
      strText = label;
      CGUIMessage msg(GUI_MSG_LABEL_SET, iParentId, iControlId);
      msg.SetLabel(strText);
      g_windowManager.SendThreadMessage(msg, iParentId);
    }

    String ControlEdit::getLabel()
    {
      if (!pGUIControl)
        return NULL;
      return strText;
    }

    void ControlEdit::setText(const String& text)
    {
      // create message
      CGUIMessage msg(GUI_MSG_LABEL2_SET, iParentId, iControlId);
      msg.SetLabel(text);

      // send message
      g_windowManager.SendThreadMessage(msg, iParentId);
    }

    String ControlEdit::getText()
    {
      CGUIMessage msg(GUI_MSG_ITEM_SELECTED, iParentId, iControlId);
      g_windowManager.SendMessage(msg, iParentId);

      return msg.GetLabel();
    }

    // ============================================================
    //  ControlList
    // ============================================================
    ControlList::ControlList(long x, long y, long width, long height, const char* font,
                             const char* ctextColor, const char* cbuttonTexture,
                             const char* cbuttonFocusTexture,
                             const char* cselectedColor,
                             long _imageWidth, long _imageHeight, long _itemTextXOffset,
                             long _itemTextYOffset, long _itemHeight, long _space, long _alignmentY) :
      strFont("font13"),
      textColor(0xe0f0f0f0), selectedColor(0xffffffff),
      imageHeight(_imageHeight), imageWidth(_imageWidth),
      itemHeight(_itemHeight), space(_space),
      itemTextOffsetX(_itemTextXOffset),itemTextOffsetY(_itemTextYOffset),
      alignmentY(_alignmentY)
    {
      dwPosX = x;
      dwPosY = y;
      dwWidth = width;
      dwHeight = height;

      // create a python spin control
      pControlSpin = new ControlSpin();

      // initialize default values
      if (font)
        strFont = font;

      if (ctextColor)
        sscanf( ctextColor, "%x", &textColor );

      if (cselectedColor)
        sscanf( cselectedColor, "%x", &selectedColor );

      strTextureButton = cbuttonTexture ? cbuttonTexture :
        XBMCAddonUtils::getDefaultImage((char*)"listcontrol", (char*)"texturenofocus", (char*)"list-nofocus.png");

      strTextureButtonFocus = cbuttonFocusTexture ? cbuttonFocusTexture :
        XBMCAddonUtils::getDefaultImage((char*)"listcontrol", (char*)"texturefocus", (char*)"list-focus.png");

      // default values for spin control
      pControlSpin->dwPosX = dwWidth - 35;
      pControlSpin->dwPosY = dwHeight - 15;
    }

    ControlList::~ControlList() { }

    CGUIControl* ControlList::Create()
    {
      CLabelInfo label;
      label.align = alignmentY;
      label.font = g_fontManager.GetFont(strFont);
      label.textColor = label.focusedColor = textColor;
      //label.shadowColor = shadowColor;
      label.selectedColor = selectedColor;
      label.offsetX = (float)itemTextOffsetX;
      label.offsetY = (float)itemTextOffsetY;
      // Second label should have the same font, alignment, and colours as the first, but
      // the offsets should be 0.
      CLabelInfo label2 = label;
      label2.offsetX = label2.offsetY = 0;
      label2.align |= XBFONT_RIGHT;

      pGUIControl = new CGUIListContainer(
        iParentId,
        iControlId,
        (float)dwPosX,
        (float)dwPosY,
        (float)dwWidth,
        (float)dwHeight - pControlSpin->dwHeight - 5,
        label, label2,
        CTextureInfo(strTextureButton),
        CTextureInfo(strTextureButtonFocus),
        (float)itemHeight,
        (float)imageWidth, (float)imageHeight,
        (float)space);

      return pGUIControl;
    }

    void ControlList::addItem(const Alternative<String, const XBMCAddon::xbmcgui::ListItem* > & item, bool sendMessage)
    {
      XBMC_TRACE;

      if (item.which() == first)
        internAddListItem(ListItem::fromString(item.former()),sendMessage);
      else
        internAddListItem(item.later(),sendMessage);
    }

    void ControlList::addItems(const std::vector<Alternative<String, const XBMCAddon::xbmcgui::ListItem* > > & items)
    {
      XBMC_TRACE;

      for (std::vector<Alternative<String, const XBMCAddon::xbmcgui::ListItem* > >::const_iterator iter = items.begin(); iter != items.end(); ++iter)
        addItem(*iter,false);
      sendLabelBind(vecItems.size());
    }

    void ControlList::internAddListItem(AddonClass::Ref<ListItem> pListItem, bool sendMessage)
    {
      if (pListItem.isNull())
        throw WindowException("NULL ListItem passed to ControlList::addListItem");

      // add item to objects vector
      vecItems.push_back(pListItem);

      // send all of the items ... this is what it did before.
      if (sendMessage)
        sendLabelBind(vecItems.size());
    }

    void ControlList::sendLabelBind(int tail)
    {
      // construct a CFileItemList to pass 'em on to the list
      CGUIListItemPtr items(new CFileItemList());
      for (unsigned int i = vecItems.size() - tail; i < vecItems.size(); i++)
        ((CFileItemList*)items.get())->Add(vecItems[i]->item);

      CGUIMessage msg(GUI_MSG_LABEL_BIND, iParentId, iControlId, 0, 0, items);
      msg.SetPointer(items.get());
      g_windowManager.SendThreadMessage(msg, iParentId);
    }

    void ControlList::selectItem(long item)
    {
      // create message
      CGUIMessage msg(GUI_MSG_ITEM_SELECT, iParentId, iControlId, item);

      // send message
      g_windowManager.SendThreadMessage(msg, iParentId);
    }

    void ControlList::removeItem(int index)
    {
      if (index < 0 || index >= (int)vecItems.size())
        throw WindowException("Index out of range");

      vecItems.erase(vecItems.begin() + index);

      sendLabelBind(vecItems.size());
    }

    void ControlList::reset()
    {
      CGUIMessage msg(GUI_MSG_LABEL_RESET, iParentId, iControlId);

      g_windowManager.SendThreadMessage(msg, iParentId);

      // delete all items from vector
      // delete all ListItem from vector
      vecItems.clear(); // this should delete all of the objects
    }

    Control* ControlList::getSpinControl()
    {
      return pControlSpin;
    }

    long ControlList::getSelectedPosition()
    {
      DelayedCallGuard dcguard(languageHook);
      LOCKGUI;

      // create message
      CGUIMessage msg(GUI_MSG_ITEM_SELECTED, iParentId, iControlId);
      long pos = -1;

      // send message
      if ((vecItems.size() > 0) && pGUIControl)
      {
        pGUIControl->OnMessage(msg);
        pos = msg.GetParam1();
      }

      return pos;
    }

    XBMCAddon::xbmcgui::ListItem* ControlList::getSelectedItem()
    {
      DelayedCallGuard dcguard(languageHook);
      LOCKGUI;

      // create message
      CGUIMessage msg(GUI_MSG_ITEM_SELECTED, iParentId, iControlId);
      AddonClass::Ref<ListItem> pListItem = NULL;

      // send message
      if ((vecItems.size() > 0) && pGUIControl)
      {
        pGUIControl->OnMessage(msg);
        if (msg.GetParam1() >= 0 && (size_t)msg.GetParam1() < vecItems.size())
          pListItem = vecItems[msg.GetParam1()];
      }

      return pListItem.get();
    }

    void ControlList::setImageDimensions(long imageWidth,long imageHeight)
    {
      CLog::Log(LOGWARNING,"ControlList::setImageDimensions was called but ... it currently isn't defined to do anything.");
      /*
        PyXBMCGUILock();
        if (self->pGUIControl)
        {
        CGUIListControl* pListControl = (CGUIListControl*) self->pGUIControl;
        pListControl->SetImageDimensions((float)self->dwImageWidth, (float)self->dwImageHeight );
        }
        PyXBMCGUIUnlock();
      */
    }

    void ControlList::setItemHeight(long height)
    {
      CLog::Log(LOGWARNING,"ControlList::setItemHeight was called but ... it currently isn't defined to do anything.");
      /*
        PyXBMCGUILock();
        if (self->pGUIControl)
        {
        CGUIListControl* pListControl = (CGUIListControl*) self->pGUIControl;
        pListControl->SetItemHeight((float)self->dwItemHeight);
        }
        PyXBMCGUIUnlock();
      */
    }

    void ControlList::setSpace(int space)
    {
      CLog::Log(LOGWARNING,"ControlList::setSpace was called but ... it currently isn't defined to do anything.");
      /*
        PyXBMCGUILock();
        if (self->pGUIControl)
        {
        CGUIListControl* pListControl = (CGUIListControl*) self->pGUIControl;
        pListControl->SetSpaceBetweenItems((float)self->dwSpace);
        }
        PyXBMCGUIUnlock();
      */
    }

    void ControlList::setPageControlVisible(bool visible)
    {
      CLog::Log(LOGWARNING,"ControlList::setPageControlVisible was called but ... it currently isn't defined to do anything.");

      //      char isOn = true;

      /*
        PyXBMCGUILock();
        if (self->pGUIControl)
        {
        ((CGUIListControl*)self->pGUIControl)->SetPageControlVisible((bool)isOn );
        }
        PyXBMCGUIUnlock();
      */
    }

    long ControlList::size()
    {
      return (long)vecItems.size();
    }

    long ControlList::getItemHeight()
    {
      return (long)itemHeight;
    }

    long ControlList::getSpace()
    {
      return (long)space;
    }

    XBMCAddon::xbmcgui::ListItem* ControlList::getListItem(int index)
    {
      if (index < 0 || index >= (int)vecItems.size())
        throw WindowException("Index out of range");

      AddonClass::Ref<ListItem> pListItem = vecItems[index];
      return pListItem.get();
    }

    void ControlList::setStaticContent(const ListItemList* pitems)
    {
      const ListItemList& vecItems = *pitems;

      std::vector<CGUIStaticItemPtr> items;

      for (unsigned int item = 0; item < vecItems.size(); item++)
      {
        ListItem* pItem = vecItems[item];

        // NOTE: This code has likely not worked fully correctly for some time
        //       In particular, the click behaviour won't be working.
        CGUIStaticItemPtr newItem(new CGUIStaticItem(*pItem->item));
        items.push_back(newItem);
      }

      // set static list
      IListProvider *provider = new CStaticListProvider(items);
      ((CGUIBaseContainer *)pGUIControl)->SetListProvider(provider);
    }

    // ============================================================

  }
}
