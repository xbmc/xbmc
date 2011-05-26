#pragma once
/*
 *      Copyright (C) 2005-2011 Team XBMC
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
#include <string>
#include <vector>
#include <ctime>
#include "uri.h"

using namespace std;

#define delete_null(ptr) (delete (ptr), ptr = NULL)

/**
 * String tokenize
 * Split string using the given delimiter into a vector of substrings
 */
void Tokenize(const string& str, vector<string>& tokens, const string& delimiters);

std::wstring StringToWString(const std::string& s);
std::string WStringToString(const std::wstring& s);
std::string lowercase(const std::string& s);
bool stringtobool(const std::string& s);
