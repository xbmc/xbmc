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

MP4Mp4vAtom::MP4Mp4vAtom() 
	: MP4Atom("mp4v")
{
	AddReserved("reserved1", 6); /* 0 */

	AddProperty( /* 1 */
		new MP4Integer16Property("dataReferenceIndex"));

	AddReserved("reserved2", 16); /* 2 */

	AddProperty( /* 3 */
		new MP4Integer16Property("width"));
	AddProperty( /* 4 */
		new MP4Integer16Property("height"));

	AddReserved("reserved3", 14); /* 5 */

	MP4StringProperty* pProp = 
		new MP4StringProperty("compressorName");
	pProp->SetFixedLength(32);
	pProp->SetValue("");
	AddProperty(pProp); /* 6 */

	AddReserved("reserved4", 4); /* 7 */

	ExpectChildAtom("esds", Required, OnlyOne);
}

void MP4Mp4vAtom::Generate()
{
	MP4Atom::Generate();

	((MP4Integer16Property*)m_pProperties[1])->SetValue(1);

	// property reserved3 has non-zero fixed values
	static u_int8_t reserved3[14] = {
		0x00, 0x48, 0x00, 0x00, 
		0x00, 0x48, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x01,
	};
	m_pProperties[5]->SetReadOnly(false);
	((MP4BytesProperty*)m_pProperties[5])->
		SetValue(reserved3, sizeof(reserved3));
	m_pProperties[5]->SetReadOnly(true);

	// property reserved4 has non-zero fixed values
	static u_int8_t reserved4[4] = {
		0x00, 0x18, 0xFF, 0xFF, 
	};
	m_pProperties[7]->SetReadOnly(false);
	((MP4BytesProperty*)m_pProperties[7])->
		SetValue(reserved4, sizeof(reserved4));
	m_pProperties[7]->SetReadOnly(true);
}

