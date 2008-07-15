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
#include "Neptune.h"

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
class PLT_StateVariable;
class PLT_Argument;
class PLT_ActionDesc;
typedef NPT_Array<PLT_Argument*> PLT_Arguments;

/*----------------------------------------------------------------------
|   PLT_ArgumentDesc
+---------------------------------------------------------------------*/
class PLT_ArgumentDesc
{
public:
    PLT_ArgumentDesc(const char*        name,
                     const char*        dir = "in",                  
                     PLT_StateVariable* variable = NULL, 
                     bool               has_ret = false);

    // accessor methods
    NPT_Result         GetSCPDXML(NPT_XmlElementNode* node);
    const NPT_String&  GetName() const {return m_Name;}
    const NPT_String&  GetDirection() const {return m_Direction;}
    PLT_StateVariable* GetRelatedStateVariable() {return m_RelatedStateVariable;}
    bool               HasRetValue() {return m_HasReturnValue;}

protected:
    NPT_String         m_Name;
    NPT_String         m_Direction;
    PLT_StateVariable* m_RelatedStateVariable;
    bool               m_HasReturnValue;
};

/*----------------------------------------------------------------------
|   PLT_Argument
+---------------------------------------------------------------------*/
class PLT_Argument
{
public:
    PLT_Argument(PLT_ArgumentDesc* arg_desc);

    // class methods
    static NPT_Result CreateArgument(PLT_ActionDesc* action_desc, 
                                     const char*     arg_name, 
                                     const char*     arg_value,
                                     PLT_Argument*&  arg);

    // accessor methods
    PLT_ArgumentDesc*  GetDesc() { return m_ArgDesc; }
    NPT_Result         SetValue(const char* value);
    const NPT_String&  GetValue();

private:
    NPT_Result         ValidateValue(const char* value);
    
protected:
    PLT_ArgumentDesc*  m_ArgDesc;
    NPT_String         m_Value;
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
        return argument->GetDesc()->GetName().Compare(m_Name, true) ? false : true;
    }

private:
    // members
    NPT_String m_Name;
};

/*----------------------------------------------------------------------
|   PLT_ArgumentDescNameFinder
+---------------------------------------------------------------------*/
class PLT_ArgumentDescNameFinder
{
public:
    // methods
    PLT_ArgumentDescNameFinder(const char* name) : m_Name(name) {}

    bool operator()(PLT_ArgumentDesc* const & arg_desc) const {
        return arg_desc->GetName().Compare(m_Name, true) ? false : true;
    }

private:
    // members
    NPT_String m_Name;
};

#endif /* _PLT_ARGUMENT_H_ */
