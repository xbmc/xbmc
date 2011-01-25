/*****************************************************************
|
|   Platinum - Service Action
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

#ifndef _PLT_ACTION_H_
#define _PLT_ACTION_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Neptune.h"
#include "PltArgument.h"
#include "PltDeviceData.h"

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
class PLT_Service;

/*----------------------------------------------------------------------
|   PLT_ActionDesc
+---------------------------------------------------------------------*/
class PLT_ActionDesc
{
public:
    PLT_ActionDesc(const char* name, PLT_Service* service);
   ~PLT_ActionDesc();

    NPT_Array<PLT_ArgumentDesc*>& GetArgumentDescs() { 
        return m_ArgumentDescs; 
    }

    const NPT_String& GetName() const { return m_Name;}
    PLT_ArgumentDesc* GetArgumentDesc(const char* name);
    NPT_Result        GetSCPDXML(NPT_XmlElementNode* node);
    PLT_Service*      GetService();

protected:
    //members
    NPT_String                   m_Name;
    PLT_Service*                 m_Service;
    NPT_Array<PLT_ArgumentDesc*> m_ArgumentDescs;
};

/*----------------------------------------------------------------------
|   PLT_Action
+---------------------------------------------------------------------*/
class PLT_Action
{
public:
   ~PLT_Action();

    PLT_ActionDesc& GetActionDesc() { return m_ActionDesc; }
    
    NPT_Result    GetArgumentValue(const char* name, NPT_String& value);
    NPT_Result    GetArgumentValue(const char* name, NPT_UInt32& value);
    NPT_Result    GetArgumentValue(const char* name, NPT_Int32& value);
	NPT_Result    GetArgumentValue(const char* name, bool& value);
    NPT_Result    VerifyArgumentValue(const char* name, const char* value);
    NPT_Result    VerifyArguments(bool input);
    NPT_Result    SetArgumentOutFromStateVariable(const char* name);
    NPT_Result    SetArgumentsOutFromStateVariable();
    NPT_Result    SetArgumentValue(const char* name, const char* value);

    NPT_Result    SetError(unsigned int code, const char* description);
    const char*   GetError(unsigned int& code);
    unsigned int  GetErrorCode();
    
    NPT_Result    FormatSoapRequest(NPT_OutputStream& stream);
    NPT_Result    FormatSoapResponse(NPT_OutputStream& stream);

    static NPT_Result FormatSoapError(unsigned int      code, 
                                      NPT_String        desc, 
                                      NPT_OutputStream& stream);

private:
    // only PLT_DeviceHost and PLT_CtrlPoint can instantiate those directly
    friend class PLT_DeviceHost;
    friend class PLT_CtrlPoint;
    PLT_Action(PLT_ActionDesc& action_desc);
    PLT_Action(PLT_ActionDesc& action_desc, PLT_DeviceDataReference& root_device);

    NPT_Result    SetArgumentOutFromStateVariable(PLT_ArgumentDesc* arg_desc);
    PLT_Argument* GetArgument(const char* name);

protected:
    // members
    PLT_ActionDesc&         m_ActionDesc;
    PLT_Arguments           m_Arguments;
    unsigned int            m_ErrorCode;
    NPT_String              m_ErrorDescription;
    // keep reference to device to prevent it 
    // from being released during action lifetime
	PLT_DeviceDataReference m_RootDevice;
};

typedef NPT_Reference<PLT_Action> PLT_ActionReference;

/*----------------------------------------------------------------------
|   PLT_GetSCPDXMLIterator
+---------------------------------------------------------------------*/
template <class T>
class PLT_GetSCPDXMLIterator
{
public:
    PLT_GetSCPDXMLIterator<T>(NPT_XmlElementNode* node) :
        m_Node(node) {}
      
    NPT_Result operator()(T* const & data) const {
        return data->GetSCPDXML(m_Node);
    }

private:
    NPT_XmlElementNode* m_Node;
};

/*----------------------------------------------------------------------
|   PLT_ActionDescNameFinder
+---------------------------------------------------------------------*/
class PLT_ActionDescNameFinder
{
public:
    // methods
    PLT_ActionDescNameFinder(PLT_Service* service, const char* name) : 
        m_Service(service), m_Name(name) {}
    virtual ~PLT_ActionDescNameFinder() {}

    bool operator()(const PLT_ActionDesc* const & action_desc) const {
        return action_desc->GetName().Compare(m_Name, true) ? false : true;
    }

private:
    // members
    PLT_Service* m_Service;
    NPT_String   m_Name;
};

#endif /* _PLT_ACTION_H_ */
