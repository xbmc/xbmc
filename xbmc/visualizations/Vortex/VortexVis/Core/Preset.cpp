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

#include "Preset.h"
#include <stdio.h>
#include <string.h>
#include "DebugConsole.h"
#include "angelscript.h"

const char * constants = 
"const int PRIM_POINTLIST     = 1;"
"const int PRIM_LINELIST      = 2;"
"const int PRIM_LINESTRIP     = 3;"
"const int PRIM_TRIANGLELIST  = 4;"
"const int PRIM_TRIANGLESTRIP = 5;"
"const int PRIM_TRIANGLEFAN   = 6;"
"const int PRIM_LINELOOP      = 7;"
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
void Preset::Init( asIScriptEngine* pScriptEngine, char* pModuleName )
{
	m_pScriptEngine = pScriptEngine;
	m_moduleName[ 0 ] = 0;
	strncpy_s( m_moduleName, 32, pModuleName, 31 );
	m_moduleName[ 31 ] = 0;
	m_bValid = false;

} // Init

//-- End --------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Preset::End()
{
	if ( m_bValid )
	{
		m_pContext->Release();
		m_pScriptEngine->DiscardModule( m_moduleName );
	}

	m_bValid = false;

} // End

//-- Begin --------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
bool Preset::Begin( char* pFilename )
{
	int r;
	char* pScript;
	FILE* iS;

	m_bValid = false;

	DebugConsole::Log("INFO: Loading preset %s\n", pFilename );

	//-------------------------------------------------------------------------
	// Load script
	iS = fopen( pFilename, "rb" );
	if ( iS == NULL )
	{
		DebugConsole::Error( "ERROR: Unable to open preset %s\n", pFilename );
		return false;
	}

	fseek(iS, 0, SEEK_END);
	int length = ftell(iS);
	fseek(iS, 0, SEEK_SET);
	pScript = new char[length];
	length = fread(pScript, 1, length, iS);
	fclose(iS);

	//-------------------------------------------------------------------------
	// Compile script
	m_pModule = m_pScriptEngine->GetModule( m_moduleName, asGM_CREATE_IF_NOT_EXISTS );
	m_pModule->AddScriptSection("constants", constants, strlen(constants), 0);
	m_pModule->AddScriptSection(m_moduleName, pScript, length, 0);

	r = m_pModule->Build();
	delete[] pScript;
	if (r < 0)
	{
		m_pScriptEngine->DiscardModule(m_moduleName);
		DebugConsole::Error("ERROR: Failed to compile Preset %s\n", pFilename);
		return false;		
	}

	//-------------------------------------------------------------------------
	// Create script context
	m_pContext = m_pScriptEngine->CreateContext();
	if (m_pContext == 0)
	{
		// Failed to create context
		m_pContext = 0;
		m_pScriptEngine->DiscardModule(m_moduleName);
		DebugConsole::Error("ERROR: Failed to create script context\n");
		return false;
	}

	//-------------------------------------------------------------------------
	// Get render function Id
	m_iRenderFuncId = m_pModule->GetFunctionIdByName("Render");

	//-------------------------------------------------------------------------
	// Execute Init function if there is one
	int initFuncId = m_pModule->GetFunctionIdByName("Init");
	if (initFuncId >= 0)
	{
		if (m_pContext->Prepare(initFuncId) == 0)
		{
			m_pContext->Execute();
		}
	}

	m_bValid = true;
	return true;

} // Begin

//-- Render -------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Preset::Render()
{
	if (!m_bValid)
		return;

	if (m_iRenderFuncId < 0)
		return;

	if (m_pContext->Prepare(m_iRenderFuncId) == 0)
	{
		m_pContext->Execute();
	}

} // Render
