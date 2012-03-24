#pragma once
/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <map>
#include <string>

typedef struct genre {
  int type;
  int subtype;
} genre_t;

typedef std::map<std::string, genre_t> GenreMap;

class CGenreTable
{
public:
  CGenreTable(const std::string &filename) { LoadGenreXML(filename); };
  bool LoadGenreXML(const std::string &filename);

  /**
   * \brief Convert a genre string into a type/subtype combination using the data in the GenreMap
   * \param strGenre (in)
   * \param genreType (out)
   * \param genreSubType (out)
   */
  void GenreToTypes(std::string& strGenre, int& genreType, int& genreSubType);
private:
  GenreMap m_genremap;
};
