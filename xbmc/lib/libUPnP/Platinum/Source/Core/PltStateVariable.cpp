/*****************************************************************
|
|   Platinum - Service State Variable
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "PltStateVariable.h"
#include "PltService.h"
#include "PltXmlHelper.h"
#include "PltUPnP.h"

NPT_SET_LOCAL_LOGGER("platinum.core.statevariable")

/*----------------------------------------------------------------------
|   PLT_StateVariable::PLT_StateVariable
+---------------------------------------------------------------------*/
PLT_StateVariable::PLT_StateVariable(PLT_Service* service) : 
    m_Service(service), 
    m_AllowedValueRange(NULL) 
{
}

/*----------------------------------------------------------------------
|   PLT_StateVariable::~PLT_StateVariable
+---------------------------------------------------------------------*/
PLT_StateVariable::~PLT_StateVariable() 
{
    m_AllowedValues.Apply(NPT_ObjectDeleter<NPT_String>());
    if (m_AllowedValueRange) delete m_AllowedValueRange;
}

/*----------------------------------------------------------------------
|   PLT_StateVariable::GetSCPDXML
+---------------------------------------------------------------------*/
NPT_Result
PLT_StateVariable::GetSCPDXML(NPT_XmlElementNode* node)
{
    NPT_XmlElementNode* variable = new NPT_XmlElementNode("stateVariable");
    NPT_CHECK_SEVERE(node->AddChild(variable));

    NPT_CHECK_SEVERE(variable->SetAttribute("sendEvents", m_IsSendingEvents?"yes":"no"));
    NPT_CHECK_SEVERE(PLT_XmlHelper::AddChildText(variable, "name", m_Name));
    NPT_CHECK_SEVERE(PLT_XmlHelper::AddChildText(variable, "dataType", m_DataType));
    if (m_DefaultValue.GetLength()) {
        NPT_CHECK_SEVERE(PLT_XmlHelper::AddChildText(variable, "defaultValue", m_DefaultValue));
    }

    if (m_AllowedValues.GetItemCount()) {
        NPT_XmlElementNode* allowedValueList = new NPT_XmlElementNode("allowedValueList");
        NPT_CHECK_SEVERE(variable->AddChild(allowedValueList));
	    for( int l = 0 ; l < (int)m_AllowedValues.GetItemCount(); l++) {
            NPT_CHECK_SEVERE(PLT_XmlHelper::AddChildText(allowedValueList, "allowedValue", (*m_AllowedValues[l])));
        }
    } else if (m_AllowedValueRange) {
        NPT_XmlElementNode* range = new NPT_XmlElementNode("allowedValueRange");
        NPT_CHECK_SEVERE(variable->AddChild(range));
        NPT_CHECK_SEVERE(PLT_XmlHelper::AddChildText(range, "minimum", NPT_String::FromInteger(m_AllowedValueRange->min_value)));
        NPT_CHECK_SEVERE(PLT_XmlHelper::AddChildText(range, "maximum", NPT_String::FromInteger(m_AllowedValueRange->max_value)));
        NPT_CHECK_SEVERE(PLT_XmlHelper::AddChildText(range, "step",    NPT_String::FromInteger(m_AllowedValueRange->step)));
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_StateVariable::GetService
+---------------------------------------------------------------------*/
PLT_Service* 
PLT_StateVariable::GetService() 
{
    return m_Service;
}

/*----------------------------------------------------------------------
|   PLT_StateVariable::IsSendingEvents
+---------------------------------------------------------------------*/
bool 
PLT_StateVariable::IsSendingEvents() 
{
    return m_IsSendingEvents;
}

/*----------------------------------------------------------------------
|   PLT_StateVariable::SetValue
+---------------------------------------------------------------------*/
NPT_Result
PLT_StateVariable::SetValue(const char* value, bool publish) 
{
    if (value == NULL) {
        return NPT_FAILURE;
    }

    // update only if it's different
    if (m_Value != value) {
        NPT_Result res = ValidateValue(value);
        if (NPT_FAILED(res)) {
            return res;
        }

        m_Value = value;
        if (publish == true) {
            m_Service->AddChanged(this);
        }    
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_StateVariable::ValidateValue
+---------------------------------------------------------------------*/
NPT_Result
PLT_StateVariable::ValidateValue(const char* value)
{
    if (m_DataType.Compare("string", true) == 0) {
        // if we have a value allowed restriction, make sure the value is in our list
        if (m_AllowedValues.GetItemCount()) {
            return m_AllowedValues.Find(NPT_StringFinder(value))?NPT_SUCCESS:NPT_FAILURE;
        }
    }

    // there are more to it than allowed values, we need to test for range, etc..
    return NPT_SUCCESS;    
}
