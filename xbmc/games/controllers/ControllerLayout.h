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

#include <string>
#include <vector>

class TiXmlElement;

namespace KODI
{
namespace GAME
{
class CController;
class CControllerFeature;

class CControllerLayout
{
public:
  CControllerLayout() = default;

  void Reset(void);

  int LabelID(void) const { return m_labelId; }
  const std::string& Icon(void) const { return m_icon; }
  const std::string& Image(void) const   { return m_strImage; }
  const std::string Models() const { return m_models; }

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
   * \brief Deserialize the specified XML element
   *
   * \param pLayoutElement The XML element
   * \param controller The controller, used to obtain read-only properties
   * \param features The deserialized features, if any
   */
  void Deserialize(const TiXmlElement* pLayoutElement, const CController* controller, std::vector<CControllerFeature> &features);

private:
  const CController *m_controller = nullptr;
  int m_labelId = -1;
  std::string m_icon;
  std::string  m_strImage;
  std::string m_models;
};

}
}
