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
 *		Dave Mackie			dmackie@cisco.com
 *		Alix Marchandise-Franquet	alix@cisco.com
 */

#include "mp4common.h"

MP4StsdAtom::MP4StsdAtom() 
	: MP4Atom("stsd") 
{
	AddVersionAndFlags();

	MP4Integer32Property* pCount = 
		new MP4Integer32Property("entryCount"); 
	pCount->SetReadOnly();
	AddProperty(pCount);

	ExpectChildAtom("mp4a", Optional, Many);
	ExpectChildAtom("enca", Optional, Many);
	ExpectChildAtom("mp4s", Optional, Many);
	ExpectChildAtom("mp4v", Optional, Many);
	ExpectChildAtom("encv", Optional, Many);
	ExpectChildAtom("rtp ", Optional, Many);
}

void MP4StsdAtom::Read() 
{
	/* do the usual read */
	MP4Atom::Read();

	// check that number of children == entryCount
	MP4Integer32Property* pCount = 
		(MP4Integer32Property*)m_pProperties[2];

	if (m_pChildAtoms.Size() != pCount->GetValue()) {
		VERBOSE_READ(GetVerbosity(),
			printf("Warning: stsd inconsistency with number of entries"));

		/* fix it */
		pCount->SetReadOnly(false);
		pCount->SetValue(m_pChildAtoms.Size());
		pCount->SetReadOnly(true);
	}
}
