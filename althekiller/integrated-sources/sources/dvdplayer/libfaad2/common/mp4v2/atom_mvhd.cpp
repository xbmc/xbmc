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

MP4MvhdAtom::MP4MvhdAtom() 
	: MP4Atom("mvhd")
{
	AddVersionAndFlags();
}

void MP4MvhdAtom::AddProperties(u_int8_t version) 
{
	if (version == 1) {
		AddProperty( /* 2 */
			new MP4Integer64Property("creationTime"));
		AddProperty( /* 3 */
			new MP4Integer64Property("modificationTime"));
	} else {
		AddProperty( /* 2 */
			new MP4Integer32Property("creationTime"));
		AddProperty( /* 3 */
			new MP4Integer32Property("modificationTime"));
	}

	AddProperty( /* 4 */
		new MP4Integer32Property("timeScale"));

	if (version == 1) {
		AddProperty( /* 5 */
			new MP4Integer64Property("duration"));
	} else {
		AddProperty( /* 5 */
			new MP4Integer32Property("duration"));
	}

	MP4Float32Property* pProp;

	pProp = new MP4Float32Property("rate");
	pProp->SetFixed32Format();
	AddProperty(pProp); /* 6 */

	pProp = new MP4Float32Property("volume");
	pProp->SetFixed16Format();
	AddProperty(pProp); /* 7 */

	AddReserved("reserved1", 70); /* 8 */

	AddProperty( /* 9 */
		new MP4Integer32Property("nextTrackId"));
}

void MP4MvhdAtom::Generate() 
{
	u_int8_t version = m_pFile->Use64Bits() ? 1 : 0;
	SetVersion(version);
	AddProperties(version);

	MP4Atom::Generate();

	// set creation and modification times
	MP4Timestamp now = MP4GetAbsTimestamp();
	if (version == 1) {
		((MP4Integer64Property*)m_pProperties[2])->SetValue(now);
		((MP4Integer64Property*)m_pProperties[3])->SetValue(now);
	} else {
		((MP4Integer32Property*)m_pProperties[2])->SetValue(now);
		((MP4Integer32Property*)m_pProperties[3])->SetValue(now);
	}

	((MP4Integer32Property*)m_pProperties[4])->SetValue(1000);

	((MP4Float32Property*)m_pProperties[6])->SetValue(1.0);
	((MP4Float32Property*)m_pProperties[7])->SetValue(1.0);

	// property reserved has non-zero fixed values
	static u_int8_t reserved[70] = {
		0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x01, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x01, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x40, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
	};
	m_pProperties[8]->SetReadOnly(false);
	((MP4BytesProperty*)m_pProperties[8])->
		SetValue(reserved, sizeof(reserved));
	m_pProperties[8]->SetReadOnly(true);

	// set next track id
	((MP4Integer32Property*)m_pProperties[9])->SetValue(1);
}

void MP4MvhdAtom::Read() 
{
	/* read atom version */
	ReadProperties(0, 1);

	/* need to create the properties based on the atom version */
	AddProperties(GetVersion());

	/* now we can read the remaining properties */
	ReadProperties(1);

	Skip();	// to end of atom
}
