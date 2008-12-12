/*****************************************************************
|
|   Platinum - Service State Variable
|
|   Copyright (c) 2004-2008, Plutinosoft, LLC.
|   Author: Sylvain Rebaud (sylvain@plutinosoft.com)
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
    long min_value;
    long max_value;
    long step;
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

    const NPT_String& GetName()     const { return m_Name;     }
    const NPT_String& GetValue()    const { return m_Value;    }
    const NPT_String& GetDataType() const { return m_DataType; }

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
