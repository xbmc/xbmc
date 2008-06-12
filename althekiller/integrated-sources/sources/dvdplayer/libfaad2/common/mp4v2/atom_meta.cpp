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
 *      M. Bakker     mbakker at nero.com
 *
 * Apple iTunes META data
 */

#include "mp4common.h"

MP4MetaAtom::MP4MetaAtom()
    : MP4Atom("meta")
{
	AddVersionAndFlags(); /* 0, 1 */

    ExpectChildAtom("hdlr", Required, OnlyOne);
    ExpectChildAtom("ilst", Required, OnlyOne);
}

MP4MeanAtom::MP4MeanAtom()
    : MP4Atom("mean")
{
	AddVersionAndFlags(); /* 0, 1 */

    AddProperty(
        new MP4BytesProperty("metadata")); /* 2 */
}

void MP4MeanAtom::Read() 
{
	// calculate size of the metadata from the atom size
	((MP4BytesProperty*)m_pProperties[2])->SetValueSize(m_size - 4);

	MP4Atom::Read();
}

MP4NameAtom::MP4NameAtom()
    : MP4Atom("name")
{
	AddVersionAndFlags(); /* 0, 1 */

    AddProperty(
        new MP4BytesProperty("metadata")); /* 2 */
}

void MP4NameAtom::Read() 
{
	// calculate size of the metadata from the atom size
	((MP4BytesProperty*)m_pProperties[2])->SetValueSize(m_size - 4);

	MP4Atom::Read();
}

MP4DataAtom::MP4DataAtom()
    : MP4Atom("data")
{
	AddVersionAndFlags(); /* 0, 1 */
    AddReserved("reserved2", 4); /* 2 */

    AddProperty(
        new MP4BytesProperty("metadata")); /* 3 */
}

void MP4DataAtom::Read() 
{
	// calculate size of the metadata from the atom size
	((MP4BytesProperty*)m_pProperties[3])->SetValueSize(m_size - 8);

	MP4Atom::Read();
}

MP4IlstAtom::MP4IlstAtom()
    : MP4Atom("ilst")
{
    ExpectChildAtom("©nam", Optional, OnlyOne); /* name */
    ExpectChildAtom("©ART", Optional, OnlyOne); /* artist */
    ExpectChildAtom("©wrt", Optional, OnlyOne); /* writer */
    ExpectChildAtom("©alb", Optional, OnlyOne); /* album */
    ExpectChildAtom("©day", Optional, OnlyOne); /* date */
    ExpectChildAtom("©too", Optional, OnlyOne); /* tool */
    ExpectChildAtom("©cmt", Optional, OnlyOne); /* comment */
    ExpectChildAtom("©gen", Optional, OnlyOne); /* custom genre */
    ExpectChildAtom("trkn", Optional, OnlyOne); /* tracknumber */
    ExpectChildAtom("disk", Optional, OnlyOne); /* disknumber */
    ExpectChildAtom("gnre", Optional, OnlyOne); /* genre (ID3v1 index + 1) */
    ExpectChildAtom("cpil", Optional, OnlyOne); /* compilation */
    ExpectChildAtom("tmpo", Optional, OnlyOne); /* BPM */
    ExpectChildAtom("covr", Optional, OnlyOne); /* cover art */
    ExpectChildAtom("----", Optional, Many); /* ---- free form */
}

MP4DashAtom::MP4DashAtom()
    : MP4Atom("----")
{
    ExpectChildAtom("mean", Required, OnlyOne);
    ExpectChildAtom("name", Required, OnlyOne);
    ExpectChildAtom("data", Required, OnlyOne);
}

MP4NamAtom::MP4NamAtom()
    : MP4Atom("©nam")
{
    ExpectChildAtom("data", Required, OnlyOne);
}

MP4ArtAtom::MP4ArtAtom()
    : MP4Atom("©ART")
{
    ExpectChildAtom("data", Required, OnlyOne);
}

MP4WrtAtom::MP4WrtAtom()
    : MP4Atom("©wrt")
{
    ExpectChildAtom("data", Required, OnlyOne);
}

MP4AlbAtom::MP4AlbAtom()
    : MP4Atom("©alb")
{
    ExpectChildAtom("data", Required, OnlyOne);
}

MP4CmtAtom::MP4CmtAtom()
    : MP4Atom("©cmt")
{
    ExpectChildAtom("data", Required, OnlyOne);
}

MP4TrknAtom::MP4TrknAtom()
    : MP4Atom("trkn")
{
    ExpectChildAtom("data", Required, OnlyOne);
}

MP4DiskAtom::MP4DiskAtom()
    : MP4Atom("disk")
{
    ExpectChildAtom("data", Required, OnlyOne);
}

MP4DayAtom::MP4DayAtom()
    : MP4Atom("©day")
{
    ExpectChildAtom("data", Required, OnlyOne);
}

MP4GenAtom::MP4GenAtom()
    : MP4Atom("©gen")
{
    ExpectChildAtom("data", Required, OnlyOne);
}

MP4TooAtom::MP4TooAtom()
    : MP4Atom("©too")
{
    ExpectChildAtom("data", Required, OnlyOne);
}

MP4GnreAtom::MP4GnreAtom()
    : MP4Atom("gnre")
{
    ExpectChildAtom("data", Optional, OnlyOne);
}

MP4CpilAtom::MP4CpilAtom()
    : MP4Atom("cpil")
{
    ExpectChildAtom("data", Required, OnlyOne);
}

MP4TmpoAtom::MP4TmpoAtom()
    : MP4Atom("tmpo")
{
    ExpectChildAtom("data", Required, OnlyOne);
}

MP4CovrAtom::MP4CovrAtom()
    : MP4Atom("covr")
{
    ExpectChildAtom("data", Required, OnlyOne);
}
