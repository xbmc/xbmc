/*****************************************************************
 |
 |      Neptune - Dynamic Libraries :: Win32 Implementation
 |
 |      (c) 2001-2008 Gilles Boccon-Gibod
 |      Author: Gilles Boccon-Gibod (bok@bok.net)
 |
 ****************************************************************/

 /*----------------------------------------------------------------------
 |   includes
 +---------------------------------------------------------------------*/
#include "NptLogging.h"
#include "NptDynamicLibraries.h"

#include <windows.h>

 /*----------------------------------------------------------------------
 |   logging
 +---------------------------------------------------------------------*/
 NPT_SET_LOCAL_LOGGER("neptune.win32.dynamic-libraries")

 /*----------------------------------------------------------------------
 |   NPT_Win32DynamicLibrary
 +---------------------------------------------------------------------*/
class NPT_Win32DynamicLibrary : public NPT_DynamicLibraryInterface
{
public:
    // constructor and destructor
    NPT_Win32DynamicLibrary(HMODULE library, const char* name) : 
      m_Library(library), m_Name(name) {}

      // NPT_DynamicLibraryInterface methods
      virtual NPT_Result FindSymbol(const char* name, void*& symbol);
      virtual NPT_Result Unload();

private:
    // members
    HMODULE    m_Library;
    NPT_String m_Name;
};

/*----------------------------------------------------------------------
|   NPT_DynamicLibrary::Load
+---------------------------------------------------------------------*/
NPT_Result 
NPT_DynamicLibrary::Load(const char* name, NPT_Flags flags, NPT_DynamicLibrary*& library)
{
    NPT_COMPILER_UNUSED(flags);
    if (name == NULL) return NPT_ERROR_INVALID_PARAMETERS;

    // default return value
    library = NULL;

    // load the lib
    NPT_LOG_FINE_2("loading library %s, flags=%x", name, flags);
    HMODULE handle = LoadLibrary(name);
    if (handle == NULL) {
        NPT_LOG_FINE("library not found");
        return NPT_FAILURE;
    }

    // instantiate the object
    NPT_LOG_FINE_1("library %s loaded", name);
    library = new NPT_DynamicLibrary(new NPT_Win32DynamicLibrary(handle, name));

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_Win32DynamicLibrary::FindSymbol
+---------------------------------------------------------------------*/
NPT_Result 
NPT_Win32DynamicLibrary::FindSymbol(const char* name, void*& symbol)
{
    if (name == NULL) return NPT_ERROR_INVALID_PARAMETERS;
    symbol = NULL;
    if (m_Library == NULL) return NPT_ERROR_NO_SUCH_ITEM;

    NPT_LOG_FINE_1("finding symbol %s", name);
    symbol = GetProcAddress(m_Library, name);
    return symbol?NPT_SUCCESS:NPT_ERROR_NO_SUCH_ITEM;
}

/*----------------------------------------------------------------------
|   NPT_Win32DynamicLibrary::Unload
+---------------------------------------------------------------------*/
NPT_Result
NPT_Win32DynamicLibrary::Unload()
{
    NPT_LOG_FINE_1("unloading library %s", (const char*)m_Name);
    BOOL result = FreeLibrary(m_Library);
    if (result) {
        return NPT_SUCCESS;
    } else {
        return NPT_FAILURE;
    }
}
