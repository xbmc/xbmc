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

MP4EncaAtom::MP4EncaAtom() 
	: MP4Atom("enca") 
{
	AddReserved("reserved1", 6); /* 0 */

	AddProperty( /* 1 */
		new MP4Integer16Property("dataReferenceIndex"));

	AddReserved("reserved2", 16); /* 2 */

	AddProperty( /* 3 */
		new MP4Integer16Property("timeScale"));

	AddReserved("reserved3", 2); /* 4 */

	ExpectChildAtom("esds", Required, OnlyOne);
	ExpectChildAtom("sinf", Required, OnlyOne);
}

void MP4EncaAtom::Generate()
{
	MP4Atom::Generate();

	((MP4Integer16Property*)m_pProperties[1])->SetValue(1);

	// property reserved2 has non-zero fixed values
	static u_int8_t reserved2[16] = {
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 
		0x00, 0x02, 0x00, 0x10,
		0x00, 0x00, 0x00, 0x00, 
	};
	m_pProperties[2]->SetReadOnly(false);
	((MP4BytesProperty*)m_pProperties[2])->
		SetValue(reserved2, sizeof(reserved2));
	m_pProperties[2]->SetReadOnly(true);
}
