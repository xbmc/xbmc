/*****************************************************************
|
|   Neptune - Dynamic Libraries
|
|   (c) 2001-2008 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _NPT_DYNAMIC_LIBRARIES_H_
#define _NPT_DYNAMIC_LIBRARIES_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptTypes.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define NPT_DYANMIC_LIBRARY_LOAD_FLAG_NOW 1

/*----------------------------------------------------------------------
|   NPT_DynamicLibraryInterface
+---------------------------------------------------------------------*/
class NPT_DynamicLibraryInterface
{
public:
    virtual ~NPT_DynamicLibraryInterface() {}
    virtual NPT_Result FindSymbol(const char* name, void*& symbol) = 0;
    virtual NPT_Result Unload() = 0;
};

/*----------------------------------------------------------------------
|   NPT_DynamicLibrary
+---------------------------------------------------------------------*/
class NPT_DynamicLibrary : public NPT_DynamicLibraryInterface
{
public:
    // class methods
    static NPT_Result Load(const char* name, NPT_Flags flags, NPT_DynamicLibrary*& library);
    
    // destructor
    ~NPT_DynamicLibrary() { delete m_Delegate; }
    
    // NPT_DynamicLibraryInterface methods
    virtual NPT_Result FindSymbol(const char* name, void*& symbol) {
        return m_Delegate->FindSymbol(name, symbol);
    }
    virtual NPT_Result Unload() {
        return m_Delegate->Unload();
    }
    
private:
    // methods
    NPT_DynamicLibrary(NPT_DynamicLibraryInterface* delegate) : m_Delegate(delegate) {}
    
    // members
    NPT_DynamicLibraryInterface* m_Delegate;
};

#endif // _NPT_DYNAMIC_LIBRARIES_H_
