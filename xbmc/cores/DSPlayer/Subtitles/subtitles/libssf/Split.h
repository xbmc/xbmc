/* 
 *  Copyright (C) 2003-2006 Gabest
 *  http://www.gabest.org
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

namespace ssf
{
  class Split : public std::vector<CStdStringW>
  {
  public:
    enum SplitType {Min, Def, Max};
    Split();
    Split(LPCWSTR sep, CStdStringW str, size_t limit = 0, SplitType type = Def);
    Split(WCHAR sep, CStdStringW str, size_t limit = 0, SplitType type = Def);
    operator size_t() {return size();}
    void DoSplit(LPCWSTR sep, CStdStringW str, size_t limit, SplitType type);
    int GetAtInt(size_t i);
    float GetAtFloat(size_t i);
  };
}