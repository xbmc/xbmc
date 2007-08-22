/*****************************************************************
|
|   Platinum - Action Argument
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "PltArgument.h"
#include "PltStateVariable.h"
#include "PltXmlHelper.h"
#include "PltAction.h"

NPT_SET_LOCAL_LOGGER("platinum.core.argument")

/*----------------------------------------------------------------------
|   PLT_ArgumentDesc::PLT_ArgumentDesc
+---------------------------------------------------------------------*/
PLT_ArgumentDesc::PLT_ArgumentDesc(const char*        name, 
                                   const char*        dir, 
                                   PLT_StateVariable* variable, 
                                   bool               has_ret) :
    m_Name(name),
    m_Direction(dir), 
    m_RelatedStateVariable(variable),
    m_HasReturnValue(has_ret)
{
}

/*----------------------------------------------------------------------
|   PLT_ArgumentDesc::GetSCPDXML
+---------------------------------------------------------------------*/
NPT_Result
PLT_ArgumentDesc::GetSCPDXML(NPT_XmlElementNode* node)
{
    NPT_XmlElementNode* argument = new NPT_XmlElementNode("argument");
    NPT_CHECK_SEVERE(node->AddChild(argument));
    NPT_CHECK_SEVERE(PLT_XmlHelper::AddChildText(argument, "name", m_Name));
    NPT_CHECK_SEVERE(PLT_XmlHelper::AddChildText(argument, "direction", m_Direction));
    NPT_CHECK_SEVERE(PLT_XmlHelper::AddChildText(argument, "relatedStateVariable", m_RelatedStateVariable->GetName()));

    if (m_HasReturnValue) {
        NPT_CHECK_SEVERE(argument->AddChild(new NPT_XmlElementNode("retval")));
    }
    
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_Argument::CreateArgument
+---------------------------------------------------------------------*/
NPT_Result
PLT_Argument::CreateArgument(PLT_ActionDesc* action_desc, 
                             const char*     name, 
                             const char*     value, 
                             PLT_Argument*&  arg)
{
    // reset output params first
    arg = NULL;

    PLT_ArgumentDesc* arg_desc = action_desc->GetArgumentDesc(name);
    if (!arg_desc) return NPT_ERROR_INVALID_PARAMETERS;

    PLT_Argument* new_arg = new PLT_Argument(arg_desc);
    if (NPT_FAILED(new_arg->SetValue(value))) {
        delete new_arg;
        return NPT_ERROR_INVALID_PARAMETERS;
    }

    arg = new_arg;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_Argument::PLT_Argument
+---------------------------------------------------------------------*/
PLT_Argument::PLT_Argument(PLT_ArgumentDesc* arg_desc) :
    m_ArgDesc(arg_desc)
{

}

/*----------------------------------------------------------------------
|   PLT_Argument::SetValue
+---------------------------------------------------------------------*/
NPT_Result
PLT_Argument::SetValue(const char* value) 
{
    NPT_CHECK_SEVERE(ValidateValue(value));

    m_Value = value;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_Argument::GetValue
+---------------------------------------------------------------------*/
const NPT_String&
PLT_Argument::GetValue() 
{
    return m_Value;
}

/*----------------------------------------------------------------------
|   PLT_Argument::ValidateValue
+---------------------------------------------------------------------*/
NPT_Result
PLT_Argument::ValidateValue(const char* value)
{
    if (m_ArgDesc->GetRelatedStateVariable()) {
        return m_ArgDesc->GetRelatedStateVariable()->ValidateValue(value);
    }
    return NPT_SUCCESS;    
}
