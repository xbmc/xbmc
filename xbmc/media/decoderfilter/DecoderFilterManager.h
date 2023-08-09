/*
 *  Copyright (C) 2013-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/**
 * \file platform\DecoderFilter.h
 * \brief Declares CDecoderFilterManager which gives control about how / when to use platform decoder.
 *
 */
#include "threads/CriticalSection.h"

#include <cinttypes>
#include <set>
#include <string>

class CDVDStreamInfo;

namespace tinyxml2
{
class XMLNode;
}

/**
* @class CDecoderFilter
*
* @brief Declaration of CDecoderFilter.
*
*/
class CDecoderFilter
{
public:
  /**
  * @brief Flags to control decoder validity.
  */
  enum : uint32_t
  {
    FLAG_GENERAL_ALLOWED = 1, ///< early false exit if set
    FLAG_STILLS_ALLOWED = 2, ///< early false exit if set and stream is marked as "has stillframes"
    FLAG_DVD_ALLOWED = 4 ///< early false exit if set and stream is marked as dvd
  };

  /**
  * \fn CDecoderFilter::CDecoderFilter(const std::string& name);
  * \brief constructs a CDecoderFilter
  * \param name decodername
  * \return nothing.
  */
  CDecoderFilter(const std::string& name) : m_name(name) {}

  /**
  * \fn CDecoderFilter::CDecoderFilter(const std::string& name, uint32_t flags, uint32_t maxWidth, uint32_t maxHeight);
  * \brief constructs a CDecoderFilter
  * \param name decodername
  * \param flags collection of FLAG_ values, bitwise OR
  * \param minHeight minimum height of stream allowed by this decoder
  * \return nothing.
  */
  CDecoderFilter(const std::string& name, uint32_t flags, int minHeight);

  virtual ~CDecoderFilter() = default;

  /**
  * \fn CDecoderFilter::operator < (const CDecoderFilter& other);
  * \brief used for sorting / replacing / find
  */
  bool operator<(const CDecoderFilter& other) const { return m_name < other.m_name; }

  /**
  * \fn CDecoderFilter::isValid(const CDVDStreamInfo& streamInfo);
  * \brief test if stream is allowed by filter.
  * \return true if valid, false otherwise
  */
  virtual bool isValid(const CDVDStreamInfo& streamInfo) const;

  /**
  * \fn CDecoderFilter::Load(const XMLNode* settings);
  * \brief load all members from XML node
  * \param node filter node from where to get the values
  * \return true if operation was successful, false on error
  */
  virtual bool Load(const tinyxml2::XMLNode* node);

  /**
  * \fn CDecoderFilter::Save(XMLNode* settings);
  * \brief store all members in XML node
  * \param node a ready to use filter setting node
  * \return true if operation was successful, false on error
  */
  virtual bool Save(tinyxml2::XMLNode* node) const;

private:
  std::string m_name;

  uint32_t m_flags = 0;
  int m_minHeight = 0;
};


/**
* @class   CDecoderFilterManager
*
* @brief   Class which handles multiple CDecoderFilter elements.
*
*/

class CDecoderFilterManager
{
public:
  CDecoderFilterManager() { Load(); }
  virtual ~CDecoderFilterManager() { Save(); }

  /**
  * \fn bool CDecoderFilterManager::add(const CDecoderFilter& filter);
  * \brief adds an CDecoderFilter if key [filter.name] is not yet existing.
  * \param filter the decoder filter to add / replace.
  * \return nothing.
  */
  void add(const CDecoderFilter& filter);


  /**
  * \fn bool CDecoderFilterManager::validate(const std::string& name, const CDVDStreamInfo& streamInfo);
  * \brief Validates if decoder with name [name] is allowed to be used.
  * \param streamInfo    Stream information used to validate().
  * \return true if HardwarDecoder could be used, false otherwise.
  */
  bool isValid(const std::string& name, const CDVDStreamInfo& streamInfo);

protected:
  bool Load();
  bool Save() const;

private:
  bool m_dirty = false;
  std::set<CDecoderFilter> m_filters;
  mutable CCriticalSection m_critical;
};
