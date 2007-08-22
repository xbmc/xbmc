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

MP4TrunAtom::MP4TrunAtom() 
	: MP4Atom("trun")
{
	AddVersionAndFlags();	/* 0, 1 */
	AddProperty( /* 2 */
		new MP4Integer32Property("sampleCount"));
}

void MP4TrunAtom::AddProperties(u_int32_t flags)
{
	if (flags & 0x01) {
		// Note this is a signed 32 value
		AddProperty(
			new MP4Integer32Property("dataOffset"));
	}
	if (flags & 0x04) {
		AddProperty(
			new MP4Integer32Property("firstSampleFlags"));
	}

	MP4TableProperty* pTable = 
		new MP4TableProperty("samples", m_pProperties[2]);
	AddProperty(pTable);

	if (flags & 0x100) {
		pTable->AddProperty(
			new MP4Integer32Property("sampleDuration"));
	}
	if (flags & 0x200) {
		pTable->AddProperty(
			new MP4Integer32Property("sampleSize"));
	}
	if (flags & 0x400) {
		pTable->AddProperty(
			new MP4Integer32Property("sampleFlags"));
	}
	if (flags & 0x800) {
		pTable->AddProperty(
			new MP4Integer32Property("sampleCompositionTimeOffset"));
	}
}

void MP4TrunAtom::Read()
{
	/* read atom version, flags, and sampleCount */
	ReadProperties(0, 3);

	/* need to create the properties based on the atom flags */
	AddProperties(GetFlags());

	/* now we can read the remaining properties */
	ReadProperties(3);

	Skip();	// to end of atom
}
