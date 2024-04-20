/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Control.h"

#include "AddonUtils.h"
#include "FileItemList.h"
#include "LanguageHook.h"
#include "ServiceBroker.h"
#include "WindowException.h"
#include "guilib/GUIButtonControl.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIControlFactory.h"
#include "guilib/GUIEditControl.h"
#include "guilib/GUIFadeLabelControl.h"
#include "guilib/GUIFontManager.h"
#include "guilib/GUIImage.h"
#include "guilib/GUILabel.h"
#include "guilib/GUILabelControl.h"
#include "guilib/GUIListContainer.h"
#include "guilib/GUIProgressControl.h"
#include "guilib/GUIRadioButtonControl.h"
#include "guilib/GUISliderControl.h"
#include "guilib/GUITextBox.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/listproviders/StaticProvider.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"

using namespace KODI;

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

      CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg, iParentId);
    }

    void ControlFadeLabel::reset()
    {
      CGUIMessage msg(GUI_MSG_LABEL_RESET, iParentId, iControlId);

      vecLabels.clear();
      CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg, iParentId);
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
        true,
        false);
      pGUIControl->SetVisible(m_visible);

      CGUIMessage msg(GUI_MSG_LABEL_RESET, iParentId, iControlId);
      pGUIControl->OnMessage(msg);

      return pGUIControl;
    }

    void ControlFadeLabel::setScrolling(bool scroll)
    {
      static_cast<CGUIFadeLabelControl*>(pGUIControl)->SetScrolling(scroll);
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
      if (pGUIControl)
      {
        // create message
        CGUIMessage msg(GUI_MSG_LABEL_SET, iParentId, iControlId);
        msg.SetLabel(text);

        // send message
        CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg, iParentId);
      }
      else
      {
        m_label = text;
      }
    }

    String ControlTextBox::getText()
    {
      if (pGUIControl == nullptr)
        return m_label;

      XBMCAddonUtils::GuiLock lock(languageHook, false);
      return static_cast<CGUITextBox*>(pGUIControl)->GetDescription();
    }

    void ControlTextBox::reset()
    {
      if (pGUIControl)
      {
        // create message
        CGUIMessage msg(GUI_MSG_LABEL_RESET, iParentId, iControlId);
        CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg, iParentId);
      }
      m_label.clear();
    }

    void ControlTextBox::scroll(long position)
    {
      if (pGUIControl)
        static_cast<CGUITextBox*>(pGUIControl)->Scroll((int)position);
    }

    void ControlTextBox::autoScroll(int delay, int time, int repeat)
    {
      if (pGUIControl)
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
      pGUIControl->SetVisible(m_visible);

      // set label
      CGUIMessage msg(GUI_MSG_LABEL_SET, iParentId, iControlId);
      msg.SetLabel(m_label);
      pGUIControl->OnMessage(msg);

      return pGUIControl;
    }

    // ============================================================

    // ============================================================
    // ============================================================
    ControlButton::ControlButton(long x,
                                 long y,
                                 long width,
                                 long height,
                                 const String& label,
                                 const char* focusTexture,
                                 const char* noFocusTexture,
                                 long _textOffsetX,
                                 long _textOffsetY,
                                 long alignment,
                                 const char* font,
                                 const char* _textColor,
                                 const char* _disabledColor,
                                 long angle,
                                 const char* _shadowColor,
                                 const char* _focusedColor)
      : textOffsetX(_textOffsetX),
        textOffsetY(_textOffsetY),
        align(alignment),
        strFont("font13"),
        textColor(0xffffffff),
        disabledColor(0x60ffffff),
        iAngle(angle),
        focusedColor(0xffffffff),
        strText(label)
    {
      dwPosX = x;
      dwPosY = y;
      dwWidth = width;
      dwHeight = height;

      // if texture is supplied use it, else get default ones
      strTextureFocus = focusTexture ? focusTexture :
        XBMCAddonUtils::getDefaultImage("button", "texturefocus");
      strTextureNoFocus = noFocusTexture ? noFocusTexture :
        XBMCAddonUtils::getDefaultImage("button", "texturenofocus");

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
        XBMCAddonUtils::GuiLock lock(languageHook, false);
        static_cast<CGUIButtonControl*>(pGUIControl)->PythonSetLabel(strFont, strText, textColor, shadowColor, focusedColor);
        static_cast<CGUIButtonControl*>(pGUIControl)->SetLabel2(strText2);
        static_cast<CGUIButtonControl*>(pGUIControl)->PythonSetDisabledColor(disabledColor);
      }
    }

    void ControlButton::setDisabledColor(const char* color)
    {
      if (color) sscanf(color, "%x", &disabledColor);

      if (pGUIControl)
      {
        XBMCAddonUtils::GuiLock lock(languageHook, false);
        static_cast<CGUIButtonControl*>(pGUIControl)->PythonSetDisabledColor(disabledColor);
      }
    }

    String ControlButton::getLabel()
    {
      if (pGUIControl == nullptr)
        return strText;

      XBMCAddonUtils::GuiLock lock(languageHook, false);
      return static_cast<CGUIButtonControl*>(pGUIControl)->GetLabel();
    }

    String ControlButton::getLabel2()
    {
      if (pGUIControl == nullptr)
        return strText2;

      XBMCAddonUtils::GuiLock lock(languageHook, false);
      return static_cast<CGUIButtonControl*>(pGUIControl)->GetLabel2();
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
      pGUIControl->SetVisible(m_visible);

      CGUIButtonControl* pGuiButtonControl =
        static_cast<CGUIButtonControl*>(pGUIControl);

      pGuiButtonControl->SetLabel(strText);
      pGuiButtonControl->SetLabel2(strText2);

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

      XBMCAddonUtils::GuiLock lock(languageHook, false);
      if (pGUIControl)
        static_cast<CGUIImage*>(pGUIControl)->SetFileName(strFileName, false, useCache);
    }

    void ControlImage::setColorDiffuse(const char* cColorDiffuse)
    {
      if (cColorDiffuse) sscanf(cColorDiffuse, "%x", &colorDiffuse);
      else colorDiffuse = 0;

      XBMCAddonUtils::GuiLock lock(languageHook, false);
      if (pGUIControl)
        static_cast<CGUIImage*>(pGUIControl)->SetColorDiffuse(GUILIB::GUIINFO::CGUIInfoColor(colorDiffuse));
    }

    CGUIControl* ControlImage::Create()
    {
      pGUIControl = new CGUIImage(iParentId, iControlId,
            (float)dwPosX, (float)dwPosY, (float)dwWidth, (float)dwHeight,
            CTextureInfo(strFileName));
      pGUIControl->SetVisible(m_visible);

      if (pGUIControl && aspectRatio <= CAspectRatio::AR_KEEP)
        static_cast<CGUIImage*>(pGUIControl)->SetAspectRatio((CAspectRatio::ASPECT_RATIO)aspectRatio);

      if (pGUIControl && colorDiffuse)
        static_cast<CGUIImage*>(pGUIControl)->SetColorDiffuse(GUILIB::GUIINFO::CGUIInfoColor(colorDiffuse));

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
        XBMCAddonUtils::getDefaultImage("progress", "texturebg");
      strTextureLeft = textureleft ? textureleft :
        XBMCAddonUtils::getDefaultImage("progress", "lefttexture");
      strTextureMid = texturemid ? texturemid :
        XBMCAddonUtils::getDefaultImage("progress", "midtexture");
      strTextureRight = textureright ? textureright :
        XBMCAddonUtils::getDefaultImage("progress", "righttexture");
      strTextureOverlay = textureoverlay ? textureoverlay :
        XBMCAddonUtils::getDefaultImage("progress", "overlaytexture");
    }

    void ControlProgress::setPercent(float pct)
    {
      if (pGUIControl)
        static_cast<CGUIProgressControl*>(pGUIControl)->SetPercentage(pct);
    }

    float ControlProgress::getPercent()
    {
      return pGUIControl ? static_cast<CGUIProgressControl*>(pGUIControl)->GetPercentage() : 0.0f;
    }

    CGUIControl* ControlProgress::Create()
    {
      pGUIControl = new CGUIProgressControl(iParentId, iControlId,
         (float)dwPosX, (float)dwPosY,
         (float)dwWidth,(float)dwHeight,
         CTextureInfo(strTextureBg), CTextureInfo(strTextureLeft),
         CTextureInfo(strTextureMid), CTextureInfo(strTextureRight),
         CTextureInfo(strTextureOverlay));
      pGUIControl->SetVisible(m_visible);

      if (pGUIControl && colorDiffuse)
        static_cast<CGUIProgressControl*>(pGUIControl)->SetColorDiffuse(GUILIB::GUIINFO::CGUIInfoColor(colorDiffuse));

      return pGUIControl;
    }

    // ============================================================

    // ============================================================
    // ============================================================
    ControlSlider::ControlSlider(long x,
                                 long y,
                                 long width,
                                 long height,
                                 const char* textureback,
                                 const char* texture,
                                 const char* texturefocus,
                                 int orientation,
                                 const char* texturebackdisabled,
                                 const char* texturedisabled)
    {
      dwPosX = x;
      dwPosY = y;
      dwWidth = width;
      dwHeight = height;
      iOrientation = orientation;

      // if texture is supplied use it, else get default ones
      strTextureBack = textureback ? textureback :
        XBMCAddonUtils::getDefaultImage("slider", "texturesliderbar");
      strTextureBackDisabled = texturebackdisabled ? texturebackdisabled : strTextureBack;
      strTexture = texture ? texture :
        XBMCAddonUtils::getDefaultImage("slider", "textureslidernib");
      strTextureFoc = texturefocus ? texturefocus :
        XBMCAddonUtils::getDefaultImage("slider", "textureslidernibfocus");
      strTextureDisabled = texturedisabled ? texturedisabled : strTexture;
    }

    float ControlSlider::getPercent()
    {
      return pGUIControl ? static_cast<CGUISliderControl*>(pGUIControl)->GetPercentage() : 0.0f;
    }

    void ControlSlider::setPercent(float pct)
    {
      if (pGUIControl)
        static_cast<CGUISliderControl*>(pGUIControl)->SetPercentage(pct);
    }

    int ControlSlider::getInt()
    {
      return (pGUIControl) ? static_cast<CGUISliderControl*>(pGUIControl)->GetIntValue() : 0;
    }

    void ControlSlider::setInt(int value, int min, int delta, int max)
    {
      if (pGUIControl)
      {
        static_cast<CGUISliderControl*>(pGUIControl)->SetType(SLIDER_CONTROL_TYPE_INT);
        static_cast<CGUISliderControl*>(pGUIControl)->SetRange(min, max);
        static_cast<CGUISliderControl*>(pGUIControl)->SetIntInterval(delta);
        static_cast<CGUISliderControl*>(pGUIControl)->SetIntValue(value);
      }
    }

    float ControlSlider::getFloat()
    {
      return (pGUIControl) ? static_cast<CGUISliderControl*>(pGUIControl)->GetFloatValue() : 0.0f;
    }

    void ControlSlider::setFloat(float value, float min, float delta, float max)
    {
      if (pGUIControl)
      {
        static_cast<CGUISliderControl*>(pGUIControl)->SetType(SLIDER_CONTROL_TYPE_FLOAT);
        static_cast<CGUISliderControl*>(pGUIControl)->SetFloatRange(min, max);
        static_cast<CGUISliderControl*>(pGUIControl)->SetFloatInterval(delta);
        static_cast<CGUISliderControl*>(pGUIControl)->SetFloatValue(value);
      }
    }

    CGUIControl* ControlSlider::Create()
    {
      pGUIControl = new CGUISliderControl(
          iParentId, iControlId, (float)dwPosX, (float)dwPosY, (float)dwWidth, (float)dwHeight,
          CTextureInfo(strTextureBack), CTextureInfo(strTextureBackDisabled),
          CTextureInfo(strTexture), CTextureInfo(strTextureFoc), CTextureInfo(strTextureDisabled),
          0, ORIENTATION(iOrientation));

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
      pGUIControl->SetVisible(m_visible);
      return pGUIControl;
    }

    // ============================================================

    // ============================================================
    // ============================================================
    ControlRadioButton::ControlRadioButton(long x, long y, long width, long height, const String& label,
                                           const char* focusOnTexture,  const char* noFocusOnTexture,
                                           const char* focusOffTexture, const char* noFocusOffTexture,
                                           const char* focusTexture, const char* noFocusTexture,
                                           long _textOffsetX, long _textOffsetY,
                                           long alignment, const char* font, const char* _textColor,
                                           const char* _disabledColor, long angle,
                                           const char* _shadowColor, const char* _focusedColor,
                                           const char* disabledOnTexture, const char* disabledOffTexture) :
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
        XBMCAddonUtils::getDefaultImage("button", "texturefocus");
      strTextureNoFocus = noFocusTexture ? noFocusTexture :
        XBMCAddonUtils::getDefaultImage("button", "texturenofocus");

      if (focusOnTexture && noFocusOnTexture)
      {
        strTextureRadioOnFocus = focusOnTexture;
        strTextureRadioOnNoFocus = noFocusOnTexture;
      }
      else
      {
        strTextureRadioOnFocus =
          XBMCAddonUtils::getDefaultImage("radiobutton", "textureradioonfocus");
        strTextureRadioOnNoFocus =
          XBMCAddonUtils::getDefaultImage("radiobutton", "textureradioonnofocus");
      }

      if (focusOffTexture && noFocusOffTexture)
      {
        strTextureRadioOffFocus = focusOffTexture;
        strTextureRadioOffNoFocus = noFocusOffTexture;
      }
      else
      {
        strTextureRadioOffFocus =
          XBMCAddonUtils::getDefaultImage("radiobutton", "textureradioofffocus");
        strTextureRadioOffNoFocus =
          XBMCAddonUtils::getDefaultImage("radiobutton", "textureradiooffnofocus");
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
        XBMCAddonUtils::GuiLock lock(languageHook, false);
        static_cast<CGUIRadioButtonControl*>(pGUIControl)->SetSelected(selected);
      }
    }

    bool ControlRadioButton::isSelected()
    {
      bool isSelected = false;

      if (pGUIControl)
      {
        XBMCAddonUtils::GuiLock lock(languageHook, false);
        isSelected = static_cast<CGUIRadioButtonControl*>(pGUIControl)->IsSelected();
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
        XBMCAddonUtils::GuiLock lock(languageHook, false);
        static_cast<CGUIRadioButtonControl*>(pGUIControl)->PythonSetLabel(strFont, strText, textColor, shadowColor, focusedColor);
        static_cast<CGUIRadioButtonControl*>(pGUIControl)->PythonSetDisabledColor(disabledColor);
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
        XBMCAddonUtils::GuiLock lock(languageHook, false);
        static_cast<CGUIRadioButtonControl*>(pGUIControl)->SetRadioDimensions((float)dwPosX, (float)dwPosY, (float)dwWidth, (float)dwHeight);
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
        CTextureInfo(strTextureRadioOffNoFocus),
        CTextureInfo(strTextureRadioOnDisabled),
        CTextureInfo(strTextureRadioOffDisabled));
      pGUIControl->SetVisible(m_visible);

      CGUIRadioButtonControl* pGuiButtonControl =
        static_cast<CGUIRadioButtonControl*>(pGUIControl);

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
      XBMCAddonUtils::GuiLock lock(languageHook, false);
      if (pGUIControl)
        pGUIControl->SetEnabled(enabled);
    }

    void Control::setVisible(bool visible)
    {
      DelayedCallGuard dcguard(languageHook);
      XBMCAddonUtils::GuiLock lock(languageHook, false);
      if (pGUIControl != nullptr)
      {
        pGUIControl->SetVisible(visible);
      }
      else
      {
        m_visible = visible;
      }
    }

    bool Control::isVisible()
    {
      DelayedCallGuard dcguard(languageHook);
      XBMCAddonUtils::GuiLock(languageHook, false);
      if (pGUIControl)
        return pGUIControl->IsVisible();
      else
        return false;
    }

    void Control::setVisibleCondition(const char* visible, bool allowHiddenFocus)
    {
      DelayedCallGuard dcguard(languageHook);
      XBMCAddonUtils::GuiLock lock(languageHook, false);

      if (pGUIControl)
        pGUIControl->SetVisibleCondition(visible, allowHiddenFocus ? "true" : "false");
    }

    void Control::setEnableCondition(const char* enable)
    {
      DelayedCallGuard dcguard(languageHook);
      XBMCAddonUtils::GuiLock lock(languageHook, false);

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
        for (const auto& i : attrs)
        {
          std::vector<std::string> attrs2 = StringUtils::Split(i, "=");
          if (attrs2.size() == 2)
            pNode.SetAttribute(attrs2[0], attrs2[1]);
        }
        TiXmlText value(cEvent.c_str());
        pNode.InsertEndChild(value);
        pRoot->InsertEndChild(pNode);
      }

      const CRect animRect((float)dwPosX, (float)dwPosY, (float)dwPosX + dwWidth, (float)dwPosY + dwHeight);
      XBMCAddonUtils::GuiLock lock(languageHook, false);
      if (pGUIControl)
      {
        CGUIControlFactory::GetAnimations(pRoot, animRect, iParentId, animations);
        pGUIControl->SetAnimations(animations);
      }
    }

    void Control::setPosition(long x, long y)
    {
      DelayedCallGuard dcguard(languageHook);
      XBMCAddonUtils::GuiLock lock(languageHook, false);
      dwPosX = x;
      dwPosY = y;
      if (pGUIControl)
        pGUIControl->SetPosition((float)dwPosX, (float)dwPosY);
    }

    void Control::setWidth(long width)
    {
      DelayedCallGuard dcguard(languageHook);
      XBMCAddonUtils::GuiLock lock(languageHook, false);
      dwWidth = width;
      if (pGUIControl)
        pGUIControl->SetWidth((float)dwWidth);
    }

    void Control::setHeight(long height)
    {
      DelayedCallGuard dcguard(languageHook);
      XBMCAddonUtils::GuiLock lock(languageHook, false);
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
        XBMCAddonUtils::GuiLock lock(languageHook, false);
        if (pGUIControl)
        {
          pGUIControl->SetAction(ACTION_MOVE_UP,    CGUIAction(up->iControlId));
          pGUIControl->SetAction(ACTION_MOVE_DOWN,  CGUIAction(down->iControlId));
          pGUIControl->SetAction(ACTION_MOVE_LEFT,  CGUIAction(left->iControlId));
          pGUIControl->SetAction(ACTION_MOVE_RIGHT, CGUIAction(right->iControlId));
        }
      }
    }

    void Control::controlUp(const Control* control)
    {
      if(iControlId == 0)
        throw WindowException("Control has to be added to a window first");

      {
        XBMCAddonUtils::GuiLock lock(languageHook, false);
        if (pGUIControl)
          pGUIControl->SetAction(ACTION_MOVE_UP, CGUIAction(control->iControlId));
      }
    }

    void Control::controlDown(const Control* control)
    {
      if(iControlId == 0)
        throw WindowException("Control has to be added to a window first");

      {
        XBMCAddonUtils::GuiLock lock(languageHook, false);
        if (pGUIControl)
          pGUIControl->SetAction(ACTION_MOVE_DOWN, CGUIAction(control->iControlId));
      }
    }

    void Control::controlLeft(const Control* control)
    {
      if(iControlId == 0)
        throw WindowException("Control has to be added to a window first");

      {
        XBMCAddonUtils::GuiLock lock(languageHook, false);
        if (pGUIControl)
          pGUIControl->SetAction(ACTION_MOVE_LEFT, CGUIAction(control->iControlId));
      }
    }

    void Control::controlRight(const Control* control)
    {
      if(iControlId == 0)
        throw WindowException("Control has to be added to a window first");

      {
        XBMCAddonUtils::GuiLock lock(languageHook, false);
        if (pGUIControl)
          pGUIControl->SetAction(ACTION_MOVE_RIGHT, CGUIAction(control->iControlId));
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
      strTextureUp = XBMCAddonUtils::getDefaultImage("listcontrol", "textureup");
      strTextureDown = XBMCAddonUtils::getDefaultImage("listcontrol", "texturedown");
      strTextureUpFocus = XBMCAddonUtils::getDefaultImage("listcontrol", "textureupfocus");
      strTextureDownFocus = XBMCAddonUtils::getDefaultImage("listcontrol", "texturedownfocus");
      strTextureUpDisabled = XBMCAddonUtils::getDefaultImage("listcontrol", "textureupdisabled");
      strTextureDownDisabled = XBMCAddonUtils::getDefaultImage("listcontrol", "texturedowndisabled");
    }

    void ControlSpin::setTextures(const char* up, const char* down,
                                  const char* upFocus,
                                  const char* downFocus,
                                  const char* upDisabled,
                                  const char* downDisabled)
    {
      strTextureUp = up;
      strTextureDown = down;
      strTextureUpFocus = upFocus;
      strTextureDownFocus = downFocus;
      strTextureUpDisabled = upDisabled;
      strTextureDownDisabled = downDisabled;
      /*
        PyXBMCGUILock();
        if (self->pGUIControl)
        {
        CGUISpinControl* pControl = (CGUISpinControl*)self->pGUIControl;
        pControl->se
        PyXBMCGUIUnlock();
      */
    }

    ControlSpin::~ControlSpin() = default;
    // ============================================================

    // ============================================================
    //  ControlLabel
    // ============================================================
    ControlLabel::ControlLabel(long x,
                               long y,
                               long width,
                               long height,
                               const String& label,
                               const char* font,
                               const char* p_textColor,
                               const char* p_disabledColor,
                               long p_alignment,
                               bool hasPath,
                               long angle)
      : strFont("font13"),
        strText(label),
        textColor(0xffffffff),
        disabledColor(0x60ffffff),
        align(p_alignment),
        bHasPath(hasPath),
        iAngle(angle)
    {
      dwPosX = x;
      dwPosY = y;
      dwWidth = width;
      dwHeight = height;

      if (font)
        strFont = font;

      if (p_textColor)
        sscanf(p_textColor, "%x", &textColor);

      if (p_disabledColor)
        sscanf( p_disabledColor, "%x", &disabledColor );
    }

    ControlLabel::~ControlLabel() = default;

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
      pGUIControl->SetVisible(m_visible);
      static_cast<CGUILabelControl*>(pGUIControl)->SetLabel(strText);
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
      CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg, iParentId);
    }

    String ControlLabel::getLabel()
    {
      return strText;
    }
    // ============================================================

    // ============================================================
    //  ControlEdit
    // ============================================================
    ControlEdit::ControlEdit(long x,
                             long y,
                             long width,
                             long height,
                             const String& label,
                             const char* font,
                             const char* _textColor,
                             const char* _disabledColor,
                             long _alignment,
                             const char* focusTexture,
                             const char* noFocusTexture)
      : strFont("font13"),
        strTextureFocus(focusTexture ? focusTexture
                                     : XBMCAddonUtils::getDefaultImage("edit", "texturefocus")),
        strTextureNoFocus(noFocusTexture
                              ? noFocusTexture
                              : XBMCAddonUtils::getDefaultImage("edit", "texturenofocus")),
        textColor(0xffffffff),
        disabledColor(0x60ffffff),
        align(_alignment)
    {
      dwPosX = x;
      dwPosY = y;
      dwWidth = width;
      dwHeight = height;

      if (!label.empty())
      {
        strText = label;
      }
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
      pGUIControl->SetVisible(m_visible);

      // set label
      CGUIMessage msg(GUI_MSG_LABEL_SET, iParentId, iControlId);
      msg.SetLabel(strText);
      CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg, iParentId);

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
      CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg, iParentId);
    }

    String ControlEdit::getLabel()
    {
      return strText;
    }

    void ControlEdit::setText(const String& text)
    {
      // create message
      CGUIMessage msg(GUI_MSG_LABEL2_SET, iParentId, iControlId);
      msg.SetLabel(text);

      // send message
      CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg, iParentId);
    }

    String ControlEdit::getText()
    {
      CGUIMessage msg(GUI_MSG_ITEM_SELECTED, iParentId, iControlId);
      CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg, iParentId);

      return msg.GetLabel();
    }

    void ControlEdit::setType(int type, const String& heading)
    {
      if (pGUIControl)
      {
        XBMCAddonUtils::GuiLock(languageHook, false);
        static_cast<CGUIEditControl*>(pGUIControl)->SetInputType(static_cast<CGUIEditControl::INPUT_TYPE>(type), CVariant{heading});
      }
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
        XBMCAddonUtils::getDefaultImage("listcontrol", "texturenofocus");

      strTextureButtonFocus = cbuttonFocusTexture ? cbuttonFocusTexture :
        XBMCAddonUtils::getDefaultImage("listcontrol", "texturefocus");

      // default values for spin control
      pControlSpin->dwPosX = dwWidth - 35;
      pControlSpin->dwPosY = dwHeight - 15;
    }

    ControlList::~ControlList() = default;

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
      pGUIControl->SetVisible(m_visible);
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

      for (const auto& iter : items)
        addItem(iter, false);
      sendLabelBind(vecItems.size());
    }

    void ControlList::internAddListItem(const AddonClass::Ref<ListItem>& pListItem,
                                        bool sendMessage)
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
      std::shared_ptr<CGUIListItem> items(new CFileItemList());
      for (unsigned int i = vecItems.size() - tail; i < vecItems.size(); i++)
        static_cast<CFileItemList*>(items.get())->Add(vecItems[i]->item);

      CGUIMessage msg(GUI_MSG_LABEL_BIND, iParentId, iControlId, 0, 0, items);
      msg.SetPointer(items.get());
      CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg, iParentId);
    }

    void ControlList::selectItem(long item)
    {
      // create message
      CGUIMessage msg(GUI_MSG_ITEM_SELECT, iParentId, iControlId, item);

      // send message
      CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg, iParentId);
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

      CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg, iParentId);

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
      XBMCAddonUtils::GuiLock lock(languageHook, false);

      // create message
      CGUIMessage msg(GUI_MSG_ITEM_SELECTED, iParentId, iControlId);
      long pos = -1;

      // send message
      if (!vecItems.empty() && pGUIControl)
      {
        pGUIControl->OnMessage(msg);
        pos = msg.GetParam1();
      }

      return pos;
    }

    XBMCAddon::xbmcgui::ListItem* ControlList::getSelectedItem()
    {
      DelayedCallGuard dcguard(languageHook);
      XBMCAddonUtils::GuiLock lock(languageHook, false);

      // create message
      CGUIMessage msg(GUI_MSG_ITEM_SELECTED, iParentId, iControlId);
      AddonClass::Ref<ListItem> pListItem = NULL;

      // send message
      if (!vecItems.empty() && pGUIControl)
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
      std::unique_ptr<IListProvider> provider = std::make_unique<CStaticListProvider>(items);
      static_cast<CGUIBaseContainer*>(pGUIControl)->SetListProvider(std::move(provider));
    }

    // ============================================================

  }
}
