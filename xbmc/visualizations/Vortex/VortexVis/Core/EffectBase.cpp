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

#include "EffectBase.h"
#include "angelscript.h"

void EffectBase::RegisterScriptInterface( class asIScriptEngine* pScriptEngine )
{
	int r;

	//---------------------------------------------------------------------------
	// Register the base effect class
	r = pScriptEngine->RegisterObjectType( "EffectBase", 0, asOBJ_REF );//	 assert(r >= 0);
	r = pScriptEngine->RegisterObjectBehaviour( "EffectBase", asBEHAVE_ADDREF, "void f()", asMETHOD( EffectBase, AddRef ), asCALL_THISCALL ); //assert(r >= 0);
	r = pScriptEngine->RegisterObjectBehaviour( "EffectBase", asBEHAVE_RELEASE, "void f()", asMETHOD( EffectBase, Release ), asCALL_THISCALL ); //assert(r >= 0);
}
