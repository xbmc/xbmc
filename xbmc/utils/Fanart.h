/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

// Fanart.h
//////////////////////////////////////////////////////////////////////

#include <string>
#include <vector>

///
/// /brief CFanart is the core of fanart support and contains all fanart data for a specific show
///
/// CFanart stores all data related to all available fanarts for a given TV show and provides
/// functions required to manipulate and access that data.
/// In order to provide an interface between the fanart data and the XBMC database, all data
/// is stored internally it its own form, as well as packed into an XML formatted string
/// stored in the member variable m_xml.
/// Information on multiple fanarts for a given show is stored, but XBMC only cares about the
/// very first fanart stored.  These interfaces provide a means to access the data in that first
/// fanart record, as well as to set which fanart is the first record.  Externally, all XBMC needs
/// to care about is getting and setting that first record.  Everything else is maintained internally
/// by CFanart.  This point is key to using the interface properly.
class CFanart
{
public:
  ///
  /// Standard constructor doesn't need to do anything
  CFanart();
  ///
  /// Takes the internal fanart data and packs it into an XML formatted string in m_xml
  /// \sa m_xml
  void Pack();
  ///
  /// Takes the XML formatted string m_xml and unpacks the fanart data contained into the internal data
  /// \return A boolean indicating success or failure
  /// \sa m_xml
  bool Unpack();
  ///
  /// Retrieves the fanart full res image URL
  /// \param index - index of image to retrieve (defaults to 0)
  /// \return A string containing the full URL to the full resolution fanart image
  std::string GetImageURL(unsigned int index = 0) const;
  ///
  /// Retrieves the fanart preview image URL, or full res image URL if that doesn't exist
  /// \param index - index of image to retrieve (defaults to 0)
  /// \return A string containing the full URL to the full resolution fanart image
  std::string GetPreviewURL(unsigned int index = 0) const;
  ///
  /// Used to return a specified fanart theme color value
  /// \param index: 0 based index of the color to retrieve.  A fanart theme contains 3 colors, indices 0-2, arranged from darkest to lightest.
  const std::string GetColor(unsigned int index) const;
  ///
  /// Sets a particular fanart to be the "primary" fanart, or in other words, sets which fanart is actually used by XBMC
  ///
  /// This is the one of the only instances in the public interface where there is any hint that more than one fanart exists, but its by necessity.
  /// \param index: 0 based index of which fanart to set as the primary fanart
  /// \return A boolean value indicating success or failure.  This should only return false if the specified index is not a valid fanart
  bool SetPrimaryFanart(unsigned int index);
  ///
  /// Returns how many fanarts are stored
  /// \return An integer indicating how many fanarts are stored in the class.  Fanart indices are 0 to (GetNumFanarts() - 1)
  unsigned int GetNumFanarts() const;
  /// Adds an image to internal fanart data
  void AddFanart(const std::string& image, const std::string& preview, const std::string& colors);
  /// Clear all internal fanart data
  void Clear();
  ///
  /// m_xml contains an XML formatted string which is all fanart packed into one string.
  ///
  /// This string is the "interface" as it were to the XBMC database, and MUST be kept in sync with the rest of the class.  Therefore
  /// anytime this string is changed, the change should be followed up by a call to CFanart::UnPack().  This XML formatted string is
  /// also the interface used to pass the fanart data from the scraper to CFanart.
  std::string m_xml;
private:
  static const unsigned int max_fanart_colors;
  ///
  /// Parse various color formats as returned by the sites scraped into a format we recognize
  ///
  /// Supported Formats:
  ///
  /// * The TVDB RGB Int Triplets, pipe separate with leading/trailing pipes "|68,69,59|69,70,58|78,78,68|"
  /// * XBMC ARGB Hexadecimal string comma separated "FFFFFFFF,DDDDDDDD,AAAAAAAA"
  ///
  /// \param colorsIn: string containing colors in some format to be converted
  /// \param colorsOut: XBMC ARGB Hexadecimal string comma separated "FFFFFFFF,DDDDDDDD,AAAAAAAA"
  /// \return boolean indicating success or failure.
  static bool ParseColors(const std::string&colorsIn, std::string&colorsOut);

  struct SFanartData
  {
    std::string strImage;
    std::string strColors;
    std::string strPreview;
  };

  ///
  /// std::vector that stores all our fanart data
  std::vector<SFanartData> m_fanart;
};

