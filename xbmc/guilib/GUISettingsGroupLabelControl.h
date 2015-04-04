#pragma once
/*
 *      Copyright (C) 2005-2015 Team XBMC
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

#include <string>

#include "GUITexture.h"
#include "GUILabel.h"
#include "GUIControl.h"

/*!
 \ingroup controls
 \brief To handle labels together with separator images inside setting groups
 */
class CGUISettingsGroupLabelControl : public CGUIControl
{
public:
  CGUISettingsGroupLabelControl(int parentID, int controlID,
                           float posX, float posY, float width, float height,
                           const CTextureInfo& texture, const CLabelInfo &label);

  virtual ~CGUISettingsGroupLabelControl(void);
  virtual CGUISettingsGroupLabelControl *Clone() const { return new CGUISettingsGroupLabelControl(*this); };

  virtual void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  virtual void Render();
  virtual bool CanFocus() const { return false; };
  virtual bool OnMessage(CGUIMessage& message);
  virtual std::string GetLabel() const { return GetDescription(); };
  virtual std::string GetDescription() const;
  virtual float GetHeight() const;
  virtual CRect CalcRenderRegion() const;
  virtual void AllocResources();
  virtual void FreeResources(bool immediately = false);
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual void SetInvalid();
  virtual void SetPosition(float posX, float posY);
  virtual void SetLabel(const std::string & aLabel);
  virtual void SetAspectRatio(const CAspectRatio &aspect);

  const CLabelInfo& GetLabelInfo() const { return m_label.GetLabelInfo(); };

  /*!
   * \brief Flags on which positions label parts allowed to be visible
   */
  enum labelAllowed
  {
    allowUndefined  = 0x00,
    allowPrimary    = 0x01,
    allowSecondary  = 0x02,
    allowInfo       = 0x04,
    allowEverywhere = 0x07
  };

  /*!
   * \brief Set the Texture really to hide, is separated to handle also usage of it
   * if no Label for first point is present on group list.
   * \param hide hide the Texture if true
   */
  void SetHideTexture(bool hide);

  /*!
   * \brief Called on creation of control with defination based upon skin xml values
   * \param alpha Set the alpha color value of the control parts
   */
  void SetAlpha(unsigned char alpha);

  /*!
   * \brief Called on creation of control with defination based upon skin xml values
   * \param fPosY Position Y offset from the top of label field
   * \param fHeight The height to which the image becomes scaled
   */
  void SetTexture(float posY, float height);

  /*!
   * \brief Called on creation of control with defination based upon skin xml values
   * \param labelAllowed contains group type flags where allowed to use the label (based upon enum labelAllowed)
   * \param textureAllowed contains group type flags where allowed to use the texture (based upon enum labelAllowed)
   * \param allowTextureFirstIfEmpty If set to true texture becomes also shown on first group without label
   * \param hideTextureIfEmpty Hide the texture if no group label is present
   */
  void SetAllowedToBeVisible(unsigned int labelAllowed, unsigned int textureAllowed, bool allowTextureFirstIfEmpty, bool hideTextureIfEmpty);

  /*!
   * \brief Called to ask for label if it is allowed to use
   * \param groupType in SettingDefinitions.h under settingsGroupType defined type to ask
   * \return true if allowed to show
   */
  bool LabelAllowedToVisible(unsigned int groupType) const;

  /*!
   * \brief Called to ask for texture if it is allowed to use
   * \param groupType in SettingDefinitions.h under settingsGroupType defined type to ask
   * \param firstGroup Set true if is first group
   * \param labelEmpty Set true if label is empty
   * \return true if allowed to show
   */
  bool TextureAllowedToVisible(unsigned int groupType, bool firstGroup, bool labelEmpty) const;

  virtual bool UpdateColors();

protected:
  virtual CGUILabel::COLOR GetTextColor() const;

  CGUITexture   m_image;
  CGUIInfoLabel m_info;
  CGUILabel     m_label;

  unsigned char m_alpha;                        //!< Alpha visible value
  float         m_imagePosY;                    //!< The offset from the top of the label for the texture
  float         m_imageHeight;                  //!< The height of the field on skin if only texture is present and no label
  float         m_normalHeight;                 //!< The normal height of the field on skin if label and texture is present
  bool          m_hideTexture;                  //!< From Settings seted flag to hide, is separated to handle groups without label to hide on first group
  unsigned int  m_settingsLabelAllowedWhere;    //!< Set with flags from "enum labelAllowed"
  unsigned int  m_settingsTextureAllowedWhere;  //!< Set with flags from "enum labelAllowed"
  bool          m_allowTextureFirstIfEmpty;     //!< If set to true texture becomes also shown on first group without label
  bool          m_hideTextureIfEmpty;           //!< If set to true the texture is hidden if no label is present
};
