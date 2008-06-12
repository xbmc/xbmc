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

#ifndef __MP4_CONTAINER_INCLUDED__
#define __MP4_CONTAINER_INCLUDED__

// base class - container of mp4 properties
class MP4Container {
public:
	MP4Container() { }

	virtual ~MP4Container();

	void AddProperty(MP4Property* pProperty);

	virtual void Read(MP4File* pFile);

	virtual void Write(MP4File* pFile);

	virtual void Dump(FILE* pFile, u_int8_t indent, bool dumpImplicits);

	MP4Property* GetProperty(u_int32_t index) {
		return m_pProperties[index];
	}

	// LATER MP4Property* GetProperty(const char* name); throw on error
	// LATER MP4Property* FindProperty(const char* name, u_int32_t* pIndex = NULL); returns NULL on error

	bool FindProperty(const char* name, 
	  MP4Property** ppProperty, u_int32_t* pIndex = NULL);

	void FindIntegerProperty(const char* name, 
		MP4Property** ppProperty, u_int32_t* pIndex = NULL);

	u_int64_t GetIntegerProperty(const char* name);

	void SetIntegerProperty(const char* name, u_int64_t value);

	void FindFloatProperty(const char* name, 
		MP4Property** ppProperty, u_int32_t* pIndex = NULL);

	float GetFloatProperty(const char* name);

	void SetFloatProperty(const char* name, float value);

	void FindStringProperty(const char* name, 
		MP4Property** ppProperty, u_int32_t* pIndex = NULL);

	const char* GetStringProperty(const char* name);

	void SetStringProperty(const char* name, const char* value);

	void FindBytesProperty(const char* name, 
		MP4Property** ppProperty, u_int32_t* pIndex = NULL);

	void GetBytesProperty(const char* name,
		u_int8_t** ppValue, u_int32_t* pValueSize);

	void SetBytesProperty(const char* name, 
		const u_int8_t* pValue, u_int32_t valueSize);

protected:
	MP4PropertyArray	m_pProperties;
};

#endif /* __MP4_CONTAINER_INCLUDED__ */
