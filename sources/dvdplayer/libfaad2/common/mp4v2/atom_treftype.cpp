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

MP4TrefTypeAtom::MP4TrefTypeAtom(const char* type) 
	: MP4Atom(type) 
{
	MP4Integer32Property* pCount = 
		new MP4Integer32Property("entryCount"); 
	pCount->SetImplicit();
	AddProperty(pCount); /* 0 */

	MP4TableProperty* pTable = new MP4TableProperty("entries", pCount);
	AddProperty(pTable); /* 1 */

	pTable->AddProperty( /* 1, 0 */
		new MP4Integer32Property("trackId"));
}

void MP4TrefTypeAtom::Read() 
{
	// table entry count computed from atom size
	((MP4Integer32Property*)m_pProperties[0])->SetReadOnly(false);
	((MP4Integer32Property*)m_pProperties[0])->SetValue(m_size / 4);
	((MP4Integer32Property*)m_pProperties[0])->SetReadOnly(true);

	MP4Atom::Read();
}
