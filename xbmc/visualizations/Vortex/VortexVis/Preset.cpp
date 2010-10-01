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

#include "Preset.h"
#include <stdio.h>
#include <string.h>
#include <xtl.h>

const char * constants = 
"const int PRIM_POINTLIST     = 1;"
"const int PRIM_LINELIST      = 2;"
"const int PRIM_LINELOOP      = 3;"
"const int PRIM_LINESTRIP     = 4;"
"const int PRIM_TRIANGLELIST  = 5;"
"const int PRIM_TRIANGLESTRIP = 6;"
"const int PRIM_TRIANGLEFAN   = 7;"
"const int PRIM_QUADLIST      = 8;"
"const int PRIM_QUADSTRIP     = 9;"
"const int TEXTURE_FRAMEBUFFER = 1;"
"const int TEXTURE_NEXTPRESET = 2;"
"const int TEXTURE_CURRPRESET = 3;"
"const int TEXTURE_ALBUMART = 4;"
"const int NULL = 0;"
"const int BLEND_OFF = 0;"
"const int BLEND_ADD = 1;"
"const int BLEND_MOD = 2;"
"const int BLEND_MAX = 3;"
"const int FILLMODE_SOLID = 0;"
"const int FILLMODE_WIREFRAME = 1;";

//-- Init ---------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Preset_c::Init(asIScriptEngine* scriptEngine, char* moduleName)
{
	m_scriptEngine = scriptEngine;
	m_moduleName[0] = 0;
	strncpy(m_moduleName, moduleName, 31);
	m_moduleName[31] = 0;
	m_valid = false;

} // Init

//-- End --------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Preset_c::End()
{
	if (m_valid)
	{
		m_context->Release();
		m_scriptEngine->Discard(m_moduleName);
	}

	m_valid = false;

} // End

//-- Begin --------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
bool Preset_c::Begin(char* filename)
{
	int r;
	char* script;
	FILE* iS;

	m_valid = false;

	OutputDebugString("Vortex INFO: Loading preset ");
	OutputDebugString(filename);
	OutputDebugString("\n");

	//-------------------------------------------------------------------------
	// Load script
	iS = fopen(filename, "rb");
	if (iS == NULL)
	{
		OutputDebugString("Vortex ERROR: Unable to open preset\n");
		return false;
	}

	fseek(iS, 0, SEEK_END);
	int length = ftell(iS);
	fseek(iS, 0, SEEK_SET);
	script = new char[length];
	length = fread(script, 1, length, iS);
	fclose(iS);

	//-------------------------------------------------------------------------
	// Compile script
	m_scriptEngine->AddScriptSection(m_moduleName, "constants", constants, strlen(constants), false);
	m_scriptEngine->AddScriptSection(m_moduleName, m_moduleName, script, length, 0, false);

	r = m_scriptEngine->Build(m_moduleName);
	delete[] script;
	if (r < 0)
	{
		m_scriptEngine->Discard(m_moduleName);
		OutputDebugString("Vortex ERROR: Preset compilation error\n");
		return false;		
	}

	//-------------------------------------------------------------------------
	// Create script context
	m_context = m_scriptEngine->CreateContext();
	if (m_context == 0)
	{
		// Failed to create context
		m_context = 0;
		m_scriptEngine->Discard(m_moduleName);
		OutputDebugString("Vortex ERROR: Failed to create script context");
		return false;
	}

	//-------------------------------------------------------------------------
	// Get render function Id
	m_renderFuncId = m_scriptEngine->GetFunctionIDByName(m_moduleName, "Render");

	//-------------------------------------------------------------------------
	// Execute Init function if there is one
	int initFuncId = m_scriptEngine->GetFunctionIDByName(m_moduleName, "Init");
	if (initFuncId >= 0)
	{
		if (m_context->Prepare(initFuncId) == 0)
		{
			m_context->Execute();
		}
	}

	m_valid = true;
	return true;

} // Begin

//-- Render -------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Preset_c::Render()
{
	if (!m_valid)
		return;

	if (m_renderFuncId < 0)
		return;

	if (m_context->Prepare(m_renderFuncId) == 0)
	{
		m_context->Execute();
	}

} // Render