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

MP4UrnAtom::MP4UrnAtom() 
	: MP4Atom("urn ")
{
	AddVersionAndFlags();
	AddProperty(new MP4StringProperty("name"));
	AddProperty(new MP4StringProperty("location"));
}

void MP4UrnAtom::Read() 
{
	// read the version, flags, and name properties
	ReadProperties(0, 3);

	// check if location is present
	if (m_pFile->GetPosition() < GetEnd()) {
		// read it
		ReadProperties(3);
	}

	Skip();	// to end of atom
}
