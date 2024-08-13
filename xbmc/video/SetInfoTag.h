/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

class TiXmlNode;
class TiXmlElement;

class CSetInfoTag
{
public:
  CSetInfoTag() { Reset(); }
  virtual ~CSetInfoTag() = default;
  void Reset();
  /* \brief Load information to a setinfotag from an XML element

   \param element    the root XML element to parse.
   \param append     whether information should be added to the existing tag, or whether it should be reset first.
   \param prioritise if appending, whether additive tags should be prioritised (i.e. replace or prepend) over existing values. Defaults to false.

   \sa ParseNative
   */
  bool Load(const TiXmlElement* element, bool append = false, bool prioritise = false);
  bool IsEmpty() const;

  void SetOverview(const std::string& overview);
  const std::string& GetOverview() const;
  bool GetUpdateSetOverview() { return m_updateSetOverview; }

  void SetTitle(const std::string& title);
  const std::string& GetTitle() const;

  void Merge(const CSetInfoTag& other);
  void Copy(const CSetInfoTag& other);

private:
  std::string m_title; // Title of the movie set
  int m_id{-1}; // ID of movie set in database
  std::string m_overview; // Overview/description of the movie set
  bool m_updateSetOverview{false}; // If overview has been set

  /* \brief Parse our native XML format for video info.
   See Load for a description of the available tag types.

   \param element    the root XML element to parse.
   \param prioritise whether additive tags should be replaced (or prepended) by the content of the tags, or appended to.
   \sa Load
   */
  void ParseNative(const TiXmlElement* element, bool prioritise);
};