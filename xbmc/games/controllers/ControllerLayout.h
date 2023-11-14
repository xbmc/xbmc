/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

namespace tinyxml2
{
class XMLElement;
}

namespace KODI
{
namespace GAME
{
class CController;
class CPhysicalFeature;
class CPhysicalTopology;

/*!
 * \ingroup games
 */
class CControllerLayout
{
public:
  CControllerLayout();
  CControllerLayout(const CControllerLayout& other);
  ~CControllerLayout();

  void Reset(void);

  int LabelID(void) const { return m_labelId; }
  const std::string& Icon(void) const { return m_icon; }
  const std::string& Image(void) const { return m_strImage; }

  /*!
   * \brief Ensures the layout was deserialized correctly, and optionally logs if not
   *
   * \param bLog If true, output the cause of invalidness to the log
   *
   * \return True if the layout is valid and can be used in the GUI, false otherwise
   */
  bool IsValid(bool bLog) const;

  /*!
   * \brief Get the label of the primary layout used when mapping the controller
   *
   * \return The label, or empty if unknown
   */
  std::string Label(void) const;

  /*!
   * \brief Get the image path of the primary layout used when mapping the controller
   *
   * \return The image path, or empty if unknown
   */
  std::string ImagePath(void) const;

  /*!
   * \brief Get the physical topology of this controller
   *
   * The topology of a controller defines its ports and which controllers can
   * physically be connected to them. Also, the topology defines if the
   * controller can provide player input, which is false in the case of hubs.
   *
   * \return The physical topology of the controller
   */
  const CPhysicalTopology& Topology(void) const { return *m_topology; }

  /*!
   * \brief Deserialize the specified XML element
   *
   * \param pLayoutElement The XML element
   * \param controller The controller, used to obtain read-only properties
   * \param features The deserialized features, if any
   */
  void Deserialize(const tinyxml2::XMLElement* pLayoutElement,
                   const CController* controller,
                   std::vector<CPhysicalFeature>& features);

private:
  const CController* m_controller = nullptr;
  int m_labelId = -1;
  std::string m_icon;
  std::string m_strImage;
  std::unique_ptr<CPhysicalTopology> m_topology;
};

} // namespace GAME
} // namespace KODI
