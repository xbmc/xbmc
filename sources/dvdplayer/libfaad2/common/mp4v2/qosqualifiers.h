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

#ifndef __QOSQUALIFIERS_INCLUDED__
#define __QOSQUALIFIERS_INCLUDED__

const u_int8_t MP4QosDescrTag			 	= 0x0C; 

class MP4QosDescriptor : public MP4Descriptor {
public:
	MP4QosDescriptor();
};

typedef MP4Descriptor MP4QosQualifier;

const u_int8_t MP4QosTagsStart				= 0x01; 
const u_int8_t MP4MaxDelayQosTag			= 0x01; 
const u_int8_t MP4PrefMaxDelayQosTag		= 0x02; 
const u_int8_t MP4LossProbQosTag			= 0x03; 
const u_int8_t MP4MaxGapLossQosTag			= 0x04; 
const u_int8_t MP4MaxAUSizeQosTag			= 0x41; 
const u_int8_t MP4AvgAUSizeQosTag			= 0x42; 
const u_int8_t MP4MaxAURateQosTag			= 0x43; 
const u_int8_t MP4QosTagsEnd				= 0xFF; 

class MP4MaxDelayQosQualifier : public MP4QosQualifier {
public:
	MP4MaxDelayQosQualifier();
};

class MP4PrefMaxDelayQosQualifier : public MP4QosQualifier {
public:
	MP4PrefMaxDelayQosQualifier();
};

class MP4LossProbQosQualifier : public MP4QosQualifier {
public:
	MP4LossProbQosQualifier();
};

class MP4MaxGapLossQosQualifier : public MP4QosQualifier {
public:
	MP4MaxGapLossQosQualifier();
};

class MP4MaxAUSizeQosQualifier : public MP4QosQualifier {
public:
	MP4MaxAUSizeQosQualifier();
};

class MP4AvgAUSizeQosQualifier : public MP4QosQualifier {
public:
	MP4AvgAUSizeQosQualifier();
};

class MP4MaxAURateQosQualifier : public MP4QosQualifier {
public:
	MP4MaxAURateQosQualifier();
};

class MP4UnknownQosQualifier : public MP4QosQualifier {
public:
	MP4UnknownQosQualifier();
	void Read(MP4File* pFile);
};

#endif /* __QOSQUALIFIERS_INCLUDED__ */
