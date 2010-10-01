/*
 *      Copyright (C) 2010 Team XBMC
 *      http://xbmc.org
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef PRESET_H
#include "../angelscript/sdk/angelscript/include/angelscript.h"

class Preset_c
{
public:
	void Init(asIScriptEngine* scriptEngine, char* moduleName);
	bool Begin(char* filename);
	void Render();
	void End();

private:
	bool m_valid;
	char m_moduleName[32];
	asIScriptEngine* m_scriptEngine;
	asIScriptContext* m_context;
	int m_renderFuncId;
};

#endif