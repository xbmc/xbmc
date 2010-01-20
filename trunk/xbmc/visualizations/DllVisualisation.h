#pragma once
/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
#include "DynamicDll.h"
#include "visualizations/VisualisationTypes.h"

class DllVisualisationInterface
{
public:
  void GetModule(struct Visualisation* pScr);
};

class DllVisualisation : public DllDynamic, DllVisualisationInterface
{
  DECLARE_DLL_WRAPPER_TEMPLATE(DllVisualisation)
  DEFINE_METHOD1(void, GetModule, (struct Visualisation* p1))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD_RENAME(get_module,GetModule)
  END_METHOD_RESOLVE()
};
