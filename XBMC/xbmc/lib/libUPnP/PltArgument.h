/*****************************************************************
|
|   Platinum - Action Argument
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
 ****************************************************************/

#ifndef _PLT_ARGUMENT_H_
#define _PLT_ARGUMENT_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptDefs.h"
#include "NptXml.h"

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
class PLT_StateVariable;

/*----------------------------------------------------------------------
|   PLT_Argument class
+---------------------------------------------------------------------*/
class PLT_Argument
{
public:
    PLT_Argument(const char*        name, 
                 const char*        value = "",
                 const char*        dir = "in",                  
                 PLT_StateVariable* variable = NULL, 
                 bool               has_ret = false);

    NPT_Result         GetSCPDXML(NPT_XmlElementNode* node);
    const NPT_String&  GetName();
    PLT_StateVariable* GetRelatedStateVariable();
    void               SetValue(const char* value);
    const NPT_String&  GetValue();
    NPT_Result         ValidateValue(const char* value);
    
protected:
    friend class PLT_Action;
    friend class PLT_Service;

    NPT_String         m_Name;
    NPT_String         m_Value;
    NPT_String         m_Direction;
    PLT_StateVariable* m_RelatedStateVariable;
    bool               m_HasReturnValue;
};

/*----------------------------------------------------------------------
|   PLT_ArgumentNameFinder
+---------------------------------------------------------------------*/
class PLT_ArgumentNameFinder
{
public:
    // methods
    PLT_ArgumentNameFinder(const char* name) : m_Name(name) {}

    bool operator()(PLT_Argument* const & argument) const {
        return argument->GetName().Compare(m_Name, true) ? false : true;
    }

private:
    // members
    NPT_String   m_Name;
};

#endif /* _PLT_ARGUMENT_H_ */
