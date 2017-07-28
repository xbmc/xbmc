/*
 *      Copyright (C) 2015-2017 Team Kodi
 *      http://kodi.tv
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "ControllerFeature.h"
#include "ControllerTypes.h"
#include "addons/Addon.h"
#include "input/joysticks/JoystickTypes.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace KODI
{
namespace GAME
{
class CControllerLayout;

using JOYSTICK::FEATURE_TYPE;

class CController : public ADDON::CAddon
{
public:
  static std::unique_ptr<CController> FromExtension(ADDON::CAddonInfo addonInfo, const cp_extension_t* ext);

  CController(ADDON::CAddonInfo addonInfo);

  virtual ~CController();

  static const ControllerPtr EmptyPtr;

  /*!
   * \brief Get all controller features
   *
   * \return The features
   */
  const std::vector<CControllerFeature>& Features(void) const { return m_features; }

  /*!
   * \brief Get the count of controller features matching the specified types
   *
   * \param type The feature type, or FEATURE_TYPE::UNKNOWN to match all feature types
   * \param inputType The input type, or INPUT_TYPE::UNKNOWN to match all input types
   *
   * \return The feature count
   */
  unsigned int FeatureCount(FEATURE_TYPE type = FEATURE_TYPE::UNKNOWN,
                            JOYSTICK::INPUT_TYPE inputType = JOYSTICK::INPUT_TYPE::UNKNOWN) const;

  /*!
   * \brief Get the features matching the specified type
   *
   * \param type The feature type, or FEATURE_TYPE::UNKNOWN to get all features
   */
  void GetFeatures(std::vector<std::string>& features, FEATURE_TYPE type = FEATURE_TYPE::UNKNOWN) const;

  /*!
   * \brief Get the type of the specified feature
   *
   * \param feature The feature name to look up
   *
   * \return The feature type, or FEATURE_TYPE::UNKNOWN if an invalid feature was specified
   */
  FEATURE_TYPE FeatureType(const std::string &feature) const;

  /*!
   * \brief Get the input type of the specified feature
   *
   * \param feature The feature name to look up
   *
   * \return The input type of the feature, or INPUT_TYPE::UNKNOWN if unknown
   */
  JOYSTICK::INPUT_TYPE GetInputType(const std::string& feature) const;

  /*!
   * \brief Load the controller layout
   *
   * \return true if the layout is loaded or was already loaded, false otherwise
   */
  bool LoadLayout(void);

  /*!
   * \brief Get the primary layout
   *
   * \return The layout of the primary controller model
   */
  const CControllerLayout& Layout(void) const { return *m_layout; }

  /*!
   * \brief Get the models defined by this controller
   *
   * \return The models, or empty if no models are defined
   */
  std::vector<std::string> Models() const;

  /*!
   * \brief Get the layout for the specified model
   *
   * \param model The model name
   *
   * \return The model layout, or the primary layout if the model name is invalid
   */
  const CControllerLayout& GetModel(const std::string& model) const;

private:
  void LoadModels(const std::string &modelXmlPath);

  std::unique_ptr<CControllerLayout> m_layout;
  std::map<std::string, std::unique_ptr<CControllerLayout>> m_models;
  std::vector<CControllerFeature> m_features;
  bool m_bLoaded = false;
};

}
}
