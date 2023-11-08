/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ControllerTypes.h"
#include "addons/Addon.h"
#include "games/controllers/input/PhysicalFeature.h"
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
class CPhysicalTopology;

using JOYSTICK::FEATURE_TYPE;

/*!
 * \ingroup games
 */
class CController : public ADDON::CAddon
{
public:
  explicit CController(const ADDON::AddonInfoPtr& addonInfo);

  ~CController() override;

  static const ControllerPtr EmptyPtr;

  // Implementation of IAddon
  bool CanHaveAddonOrInstanceSettings() override { return true; }

  /*!
   * \brief Get all controller features
   *
   * \return The features
   */
  const std::vector<CPhysicalFeature>& Features(void) const { return m_features; }

  /*!
   * \brief Get a feature by its name
   *
   * \param name The feature name
   *
   * \return The feature, or a feature of type FEATURE_TYPE::UNKNOWN if the name is invalid
   */
  const CPhysicalFeature& GetFeature(const std::string& name) const;

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
  void GetFeatures(std::vector<std::string>& features,
                   FEATURE_TYPE type = FEATURE_TYPE::UNKNOWN) const;

  /*!
   * \brief Get the type of the specified feature
   *
   * \param feature The feature name to look up
   *
   * \return The feature type, or FEATURE_TYPE::UNKNOWN if an invalid feature was specified
   */
  FEATURE_TYPE FeatureType(const std::string& feature) const;

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
   * \brief Get the controller layout
   */
  const CControllerLayout& Layout(void) const { return *m_layout; }

  /*!
   * \brief Get the controller's physical topology
   *
   * This defines how controllers physically connect to each other.
   *
   * \return The physical topology of the controller
   */
  const CPhysicalTopology& Topology() const;

private:
  std::unique_ptr<CControllerLayout> m_layout;
  std::vector<CPhysicalFeature> m_features;
  bool m_bLoaded = false;
};

} // namespace GAME
} // namespace KODI
