/*****************************************************************
|
|   Neptune - Interfaces
|
|   (c) 2001-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _NPT_INTERFACES_H_
#define _NPT_INTERFACES_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptTypes.h"
#include "NptCommon.h"
#include "NptResults.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const int NPT_ERROR_NO_SUCH_INTERFACE = NPT_ERROR_BASE_INTERFACES - 0;

#if 0 // disabled, use NPT_Reference instead
/*----------------------------------------------------------------------
|   macros
+---------------------------------------------------------------------*/
#define NPT_RELEASE(o) do { if (o) (o)->Release(); (o) = NULL; } while (0)
#define NPT_ADD_REFERENCE(o) do { if (o) (o)->AddReference(); } while (0)

/*----------------------------------------------------------------------
|   NPT_Referenceable
+---------------------------------------------------------------------*/
class NPT_Referenceable
{
 public:
    // methods
    virtual void AddReference() = 0;
    virtual void Release() = 0;

protected:
    // constructors and destructor
    NPT_Referenceable() {}
    virtual ~NPT_Referenceable() {}
};
#endif

/*----------------------------------------------------------------------
|   NPT_InterfaceId
+---------------------------------------------------------------------*/
class NPT_InterfaceId
{
 public:
    // methods
    bool operator==(const NPT_InterfaceId& id) const {
        return ((id.m_Id == m_Id) && (id.m_Version == m_Version));
    }

    // members
    unsigned long m_Id;
    unsigned long m_Version;
};

/*----------------------------------------------------------------------
|   NPT_Polymorphic
+---------------------------------------------------------------------*/
class NPT_Polymorphic
{
public:
    // destructor
    virtual ~NPT_Polymorphic() {}
     
    // methods
    virtual NPT_Result GetInterface(const NPT_InterfaceId& id, 
                                    NPT_Interface*&        iface) = 0;
};

/*----------------------------------------------------------------------
|   NPT_Interruptible
+---------------------------------------------------------------------*/
class NPT_Interruptible
{
public:
    // destructor
    virtual ~NPT_Interruptible() {}

    // methods
    virtual NPT_Result Interrupt() = 0;
};

/*----------------------------------------------------------------------
|   NPT_Configurable
+---------------------------------------------------------------------*/
class NPT_Configurable
{
public:
    // destructor
    virtual ~NPT_Configurable() {}
     
    // methods
    virtual NPT_Result SetProperty(const char* /*name*/, 
                                   const char* /*value*/) { 
        return NPT_ERROR_NO_SUCH_PROPERTY;
    }
    virtual NPT_Result SetProperty(const char* /*name*/, 
                                   NPT_Integer /*value*/) { 
        return NPT_ERROR_NO_SUCH_PROPERTY;
    }
    virtual NPT_Result GetProperty(const char*        /*name*/, 
                                   NPT_PropertyValue& /*value*/) {
        return NPT_ERROR_NO_SUCH_PROPERTY;
    }
};

#endif // _NPT_INTERFACES_H_
