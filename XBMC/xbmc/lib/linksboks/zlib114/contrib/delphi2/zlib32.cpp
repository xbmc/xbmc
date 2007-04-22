
#include <windows.h>
#pragma hdrstop
#include <condefs.h>


//---------------------------------------------------------------------------
//   Important note about DLL memory management in a VCL DLL:
//
//
//
// If your DLL uses VCL and exports any functions that pass VCL String objects
// (or structs/classes containing nested Strings) as parameter or function
// results, you will need to build both your DLL project and any EXE projects
// that use your DLL with the dynamic RTL (the RTL DLL).  This will change your
// DLL and its calling EXE's to use BORLNDMM.DLL as their memory manager. In
// these cases, the file BORLNDMM.DLL should be deployed along with your DLL
// and the RTL DLL (CP3240MT.DLL). To avoid the requiring BORLNDMM.DLL in
// these situations, pass string information using "char *" or ShortString
// parameters and then link with the static RTL.
//
//---------------------------------------------------------------------------
USEUNIT("adler32.c");
USEUNIT("compress.c");
USEUNIT("crc32.c");
USEUNIT("deflate.c");
USEUNIT("gzio.c");
USEUNIT("infblock.c");
USEUNIT("infcodes.c");
USEUNIT("inffast.c");
USEUNIT("inflate.c");
USEUNIT("inftrees.c");
USEUNIT("infutil.c");
USEUNIT("trees.c");
USEUNIT("uncompr.c");
USEUNIT("zutil.c");
//---------------------------------------------------------------------------
#pragma argsused
int WINAPI DllEntryPoint(HINSTANCE hinst, unsigned long reason, void*)
{
        return 1;
}
