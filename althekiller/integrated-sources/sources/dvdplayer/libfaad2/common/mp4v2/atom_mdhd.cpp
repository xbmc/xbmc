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

MP4MdhdAtom::MP4MdhdAtom() 
	: MP4Atom("mdhd") 
{
	AddVersionAndFlags();
}

void MP4MdhdAtom::AddProperties(u_int8_t version) 
{
	if (version == 1) {
		AddProperty(
			new MP4Integer64Property("creationTime"));
		AddProperty(
			new MP4Integer64Property("modificationTime"));
	} else {
		AddProperty(
			new MP4Integer32Property("creationTime"));
		AddProperty(
			new MP4Integer32Property("modificationTime"));
	}

	AddProperty(
		new MP4Integer32Property("timeScale"));

	if (version == 1) {
		AddProperty(
			new MP4Integer64Property("duration"));
	} else {
		AddProperty(
			new MP4Integer32Property("duration"));
	}

	AddProperty(
		new MP4Integer16Property("language"));
	AddReserved("reserved", 2);
}

void MP4MdhdAtom::Generate() 
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
}

void MP4MdhdAtom::Read() 
{
	/* read atom version */
	ReadProperties(0, 1);

	/* need to create the properties based on the atom version */
	AddProperties(GetVersion());

	/* now we can read the remaining properties */
	ReadProperties(1);

	Skip();	// to end of atom
}
