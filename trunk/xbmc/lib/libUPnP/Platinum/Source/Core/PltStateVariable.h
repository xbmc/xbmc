/*****************************************************************
|
|   Platinum - Service State Variable
|
| Copyright (c) 2004-2008, Plutinosoft, LLC.
| All rights reserved.
| http://www.plutinosoft.com
|
| This program is free software; you can redistribute it and/or
| modify it under the terms of the GNU General Public License
| as published by the Free Software Foundation; either version 2
| of the License, or (at your option) any later version.
|
| OEMs, ISVs, VARs and other distributors that combine and 
| distribute commercially licensed software with Platinum software
| and do not wish to distribute the source code for the commercially
| licensed software under version 2, or (at your option) any later
| version, of the GNU General Public License (the "GPL") must enter
| into a commercial license agreement with Plutinosoft, LLC.
| 
| This program is distributed in the hope that it will be useful,
| but WITHOUT ANY WARRANTY; without even the implied warranty of
| MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
| GNU General Public License for more details.
|
| You should have received a copy of the GNU General Public License
| along with this program; see the file LICENSE.txt. If not, write to
| the Free Software Foundation, Inc., 
| 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
| http://www.gnu.org/licenses/gpl-2.0.html
|
****************************************************************/

#ifndef _PLT_STATE_VARIABLE_H_
#define _PLT_STATE_VARIABLE_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Neptune.h"

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
class PLT_Argument;
class PLT_Service;

/*----------------------------------------------------------------------
|   NPT_AllowedValueRange class
+---------------------------------------------------------------------*/
typedef struct {
    NPT_Int32 min_value;
    NPT_Int32 max_value;
    NPT_Int32 step;
} NPT_AllowedValueRange;

/*----------------------------------------------------------------------
|   PLT_StateVariable class
+---------------------------------------------------------------------*/
class PLT_StateVariable
{
public:
    PLT_StateVariable(PLT_Service* service);
    ~PLT_StateVariable();

    NPT_Result   GetSCPDXML(NPT_XmlElementNode* node);
    PLT_Service* GetService();
    bool         IsSendingEvents(bool indirectly = false);
    void         DisableIndirectEventing();
    NPT_Result   SetRate(NPT_TimeInterval rate);
    NPT_Result   SetValue(const char* value);
    NPT_Result   ValidateValue(const char* value);
	NPT_Result   Serialize(NPT_XmlElementNode& node);
	NPT_Result   SetExtraAttribute(const char* name, const char* value);

    const NPT_String& GetName()     const { return m_Name;     }
    const NPT_String& GetValue()    const { return m_Value;    }
    const NPT_String& GetDataType() const { return m_DataType; }
	const NPT_AllowedValueRange* GetAllowedValueRange() const { return m_AllowedValueRange; }

    static PLT_StateVariable* Find(NPT_List<PLT_StateVariable*>& vars, 
                                   const char*                   name);

protected:
    bool         IsReadyToPublish();

protected:
    friend class PLT_Service;

    //members
    PLT_Service*            m_Service;
    NPT_AllowedValueRange*  m_AllowedValueRange;
    NPT_String              m_Name;
    NPT_String              m_DataType;
    NPT_String              m_DefaultValue;
    bool                    m_IsSendingEvents;
    bool                    m_IsSendingEventsIndirectly;
    NPT_TimeInterval        m_Rate;
    NPT_TimeStamp           m_LastEvent;
    NPT_Array<NPT_String*>  m_AllowedValues;
    NPT_String              m_Value;

	NPT_Map<NPT_String,NPT_String> m_ExtraAttributes;
};

/*----------------------------------------------------------------------
|   PLT_StateVariableNameFinder
+---------------------------------------------------------------------*/
class PLT_StateVariableNameFinder 
{
public:
    // methods
    PLT_StateVariableNameFinder(const char* name) : m_Name(name) {}
    virtual ~PLT_StateVariableNameFinder() {}

    bool operator()(const PLT_StateVariable* const & state_variable) const {
        return state_variable->GetName().Compare(m_Name, true) ? false : true;
    }

private:
    // members
    NPT_String   m_Name;
};

/*----------------------------------------------------------------------
|   PLT_ListStateVariableNameFinder
+---------------------------------------------------------------------*/
class PLT_ListStateVariableNameFinder
{
public:
    // methods
    PLT_ListStateVariableNameFinder(const char* name) : m_Name(name) {}
    virtual ~PLT_ListStateVariableNameFinder() {}

    bool operator()(const PLT_StateVariable* const & state_variable) const {
        return state_variable->GetName().Compare(m_Name, true) ? false : true;
    }

private:
    // members
    NPT_String   m_Name;
};

#endif /* _PLT_STATE_VARIABLE_H_ */
