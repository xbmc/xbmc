/*****************************************************************
|
|   Platinum - Service Action
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
 ****************************************************************/

#ifndef _PLT_ACTION_H_
#define _PLT_ACTION_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Neptune.h"
#include "PltArgument.h"

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
    PLT_Action(PLT_ActionDesc* action_desc);
   ~PLT_Action();

    PLT_ActionDesc* GetActionDesc() { return m_ActionDesc; }
    
    NPT_Result    GetArgumentValue(const char* name, NPT_String& value);
    NPT_Result    GetArgumentValue(const char* name, NPT_UInt32& value);
    NPT_Result    GetArgumentValue(const char* name, NPT_Int32& value);
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
    NPT_Result    SetArgumentOutFromStateVariable(PLT_ArgumentDesc* arg_desc);
    PLT_Argument* GetArgument(const char* name);

protected:
    // members
    PLT_ActionDesc* m_ActionDesc;
    PLT_Arguments   m_Arguments;
    unsigned int    m_ErrorCode;
    NPT_String      m_ErrorDescription;
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
