/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is MPEG4IP.
 * 
 * The Initial Developer of the Original Code is Cisco Systems Inc.
 * Portions created by Cisco Systems Inc. are
 * Copyright (C) Cisco Systems Inc. 2001.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie		dmackie@cisco.com
 */

#include "mp4common.h"

MP4HinfAtom::MP4HinfAtom() 
	: MP4Atom("hinf")
{
	ExpectChildAtom("trpy", Optional, OnlyOne);
	ExpectChildAtom("nump", Optional, OnlyOne);
	ExpectChildAtom("tpyl", Optional, OnlyOne);
	ExpectChildAtom("maxr", Optional, Many);
	ExpectChildAtom("dmed", Optional, OnlyOne);
	ExpectChildAtom("dimm", Optional, OnlyOne);
	ExpectChildAtom("drep", Optional, OnlyOne);
	ExpectChildAtom("tmin", Optional, OnlyOne);
	ExpectChildAtom("tmax", Optional, OnlyOne);
	ExpectChildAtom("pmax", Optional, OnlyOne);
	ExpectChildAtom("dmax", Optional, OnlyOne);
	ExpectChildAtom("payt", Optional, OnlyOne);
}

void MP4HinfAtom::Generate()
{
	// hinf is special in that although all it's child atoms
	// are optional (on read), if we generate it for writing
	// we really want all the children

	for (u_int32_t i = 0; i < m_pChildAtomInfos.Size(); i++) {
		MP4Atom* pChildAtom = 
			CreateAtom(m_pChildAtomInfos[i]->m_name);

		AddChildAtom(pChildAtom);

		// and ask it to self generate
		pChildAtom->Generate();
	}
}

