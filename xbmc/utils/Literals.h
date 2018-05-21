#pragma once
/*
 *      Copyright (C) 2014 Team XBMC
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

constexpr unsigned long long int operator"" _kib (unsigned long long int val)
{
    return val * 1024ull;
}

constexpr unsigned long long int operator"" _kb (unsigned long long int val)
{
    return val * 1000ull;
}

constexpr unsigned long long int operator"" _mib (unsigned long long int val)
{
    return val * 1024ull * 1024ull;
}

constexpr unsigned long long int operator"" _mb (unsigned long long int val)
{
    return val * 1000ull * 1000ull;
}
