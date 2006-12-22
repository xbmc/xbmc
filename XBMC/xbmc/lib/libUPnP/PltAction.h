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
#include "NptDefs.h"
#include "NptStreams.h"
#include "NptXml.h"
#include "NptArray.h"

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
class PLT_StateVariable;
class PLT_Argument;
class PLT_Service;
typedef NPT_Array<PLT_Argument*> PLT_Arguments;

/*----------------------------------------------------------------------
|   PLT_Action class
+---------------------------------------------------------------------*/
class PLT_Action
{
public:
    PLT_Action(const char* name, PLT_Service* service);
   ~PLT_Action();

    const NPT_String& GetName() const { return m_Name;}

    NPT_Result    GetSCPDXML(NPT_XmlElementNode* node);
    PLT_Service*  GetService();
    NPT_Result    SetError(unsigned int code, const char* description);
    const char*   GetError(unsigned int& code);
    unsigned int  GetErrorCode();

    NPT_Result    GetArgument(const char* name, NPT_String& value);
    NPT_Result    GetArgument(const char* name, NPT_UInt32& value);
    NPT_Result    GetArgument(const char* name, NPT_Int32& value);
    PLT_Argument* GetArgument(const char* name);

    NPT_Result    SetArgument(const char* name, const char* value);
    NPT_Result    SetArgumentFromStateVariable(const char* name);
    NPT_Result    SetArgumentsIn(PLT_Arguments& args);
    NPT_Result    SetArgumentsOut(PLT_Arguments& args);
    NPT_Result    SetArgumentsOutFromStateVariable();

    NPT_Result    FormatSoapRequest(NPT_OutputStream& stream);
    NPT_Result    FormatSoapResponse(NPT_OutputStream& stream);

    static NPT_Result FormatSoapError(unsigned int      code, 
                                      NPT_String        desc, 
                                      NPT_OutputStream& stream);
    static NPT_Result AddArgument(PLT_Action*         action,
                                  PLT_Arguments&      args, 
                                  const char*         arg_name,
                                  const char*         arg_value);

protected:
    friend class PLT_Service;
    
    //members
    NPT_String           m_Name;
    PLT_Service*         m_Service;
    unsigned int         m_ErrorCode;
    PLT_Arguments        m_Arguments;
    NPT_String           m_ErrorDescription;
};

/*----------------------------------------------------------------------
|   PLT_GetSCPDXMLIterator class
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
|   PLT_ActionNameFinder
+---------------------------------------------------------------------*/
class PLT_ActionNameFinder
{
public:
    // methods
    PLT_ActionNameFinder(PLT_Service* service, const char* name) : 
      m_Service(service), m_Name(name) {}
    virtual ~PLT_ActionNameFinder() {}

    bool operator()(const PLT_Action* const & action) const {
        return action->GetName().Compare(m_Name, true) ? false : true;
    }

private:
    // members
    PLT_Service*    m_Service;
    NPT_String      m_Name;
};

#endif /* _PLT_ACTION_H_ */
