/*
 *  Copyright © 2010-2012 Team XBMC
 *  http://xbmc.org
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
 *
 */

#ifndef _PRESET_H_

class asIScriptEngine;
class asIScriptContext;
class asIScriptModule;

class Preset
{
public:
	void Init( asIScriptEngine* pScriptEngine, char* pModuleName );
	bool Begin( char* pFilename );
	void Render();
	void End();
	bool IsValid() { return m_bValid; }

	int m_presetId;

private:
	bool	m_bValid;
	char	m_moduleName[32];
	int		m_iRenderFuncId;
	asIScriptEngine* m_pScriptEngine;
	asIScriptModule* m_pModule;
	asIScriptContext* m_pContext;
};

#endif
