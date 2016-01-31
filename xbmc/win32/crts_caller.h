#pragma once
/*
 *      Copyright (C) 2015 Team Kodi
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

/**
 * \file win32\crts_caller.h
 * \brief Declares crts_caller class for calling same function for all loaded CRTs.
 * \author Karlson2k
 */

#include <string>
#include <vector>

namespace win32_utils
{

class crts_caller
{
public:
  crts_caller(const char* func_name);
  const std::vector<void*>& get_pointers(void)
  { return m_funcPointers; }
  ~crts_caller();

  template <typename ret_type, typename... param_types>
  static typename ret_type call_in_all_crts(const char* func_name, ret_type(*cur_fnc_ptr) (param_types...), param_types... params)
  {
    typedef ret_type(*ptr_type)(param_types...);

    if (cur_fnc_ptr == NULL)
      return (ret_type)0; // cur_fnc_ptr must point to process default CRT function

    crts_caller crts(func_name);
    for (void* func_ptr : crts.m_funcPointers)
    {
      ptr_type func = (ptr_type)func_ptr;
      if (func != cur_fnc_ptr)
        (void)func(params...); // ignoring result of function call
    }

    return cur_fnc_ptr(params...); // return result of calling process's CRT function
  }

  static std::vector<std::wstring> getCrtNames();
private:
  std::vector<void*> m_funcPointers;
  std::vector<void*> m_crts; // actually contains HMODULE
};

// Call function in all loaded CRTs
// Function must have same return type and same parameters in all CRTs
#define CALL_IN_CRTS(function,...) ::win32_utils::crts_caller::call_in_all_crts(#function,&(function),##__VA_ARGS__)

}
