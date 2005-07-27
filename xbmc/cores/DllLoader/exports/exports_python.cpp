#include "..\..\..\stdafx.h"
#include "..\DllLoaderContainer.h"
#include "..\..\..\lib\libpython\xbpython.h"
#include "..\..\..\lib\libpython\pythread.h"
#include "..\..\..\lib\libpython\compile.h"
#include "..\..\..\lib\libpython\frameobject.h"
#include "..\..\..\lib\libpython\symtable.h"
#include "..\..\..\lib\libpython\node.h"


int PyThread_acquire_lock(PyThread_type_lock aLock, int waitflag);
PyThread_type_lock PyThread_allocate_lock(void);
void PyThread_release_lock(PyThread_type_lock aLock);

#ifdef __cplusplus
extern "C"
{
#endif

  int PyOS_CheckStack(void);

#ifdef __cplusplus
};
#endif

void export_python23()
{
  g_dlls.python23.AddExport("PyString_FromString", (unsigned long)PyString_FromString);
  g_dlls.python23.AddExport("PyUnicodeUCS2_FromUnicode", (unsigned long)PyUnicodeUCS2_FromUnicode);
  g_dlls.python23.AddExport("Py_BuildValue", (unsigned long)Py_BuildValue);
  g_dlls.python23.AddExport("PyObject_Free", (unsigned long)PyObject_Free);
  g_dlls.python23.AddExport("PyOS_snprintf", (unsigned long)PyOS_snprintf);
  g_dlls.python23.AddExport("PyUnicodeUCS2_Resize", (unsigned long)PyUnicodeUCS2_Resize);
  g_dlls.python23.AddExport("PyCObject_FromVoidPtr", (unsigned long)PyCObject_FromVoidPtr);
  g_dlls.python23.AddExport("PyModule_AddStringConstant", (unsigned long)PyModule_AddStringConstant);
  g_dlls.python23.AddExport("Py_InitModule4", (unsigned long)Py_InitModule4);
  g_dlls.python23.AddExport("_PyUnicodeUCS2_ToNumeric", (unsigned long)_PyUnicodeUCS2_ToNumeric);
  g_dlls.python23.AddExport("PyFloat_FromDouble", (unsigned long)PyFloat_FromDouble);
  g_dlls.python23.AddExport("_PyUnicodeUCS2_ToDigit", (unsigned long)_PyUnicodeUCS2_ToDigit);
  g_dlls.python23.AddExport("PyUnicode_Type", (unsigned long)(&PyUnicode_Type));
  g_dlls.python23.AddExport("PyArg_ParseTuple", (unsigned long)PyArg_ParseTuple);
  g_dlls.python23.AddExport("PyExc_TypeError", (unsigned long)&(PyExc_TypeError));
  g_dlls.python23.AddExport("PyErr_SetString", (unsigned long)PyErr_SetString);
  g_dlls.python23.AddExport("_PyUnicodeUCS2_ToDecimalDigit", (unsigned long)_PyUnicodeUCS2_ToDecimalDigit);
  g_dlls.python23.AddExport("PyExc_ValueError", (unsigned long)(&PyExc_ValueError));
  g_dlls.python23.AddExport("PyInt_FromLong", (unsigned long)PyInt_FromLong);
  g_dlls.python23.AddExport("PyExc_KeyError", (unsigned long)(&PyExc_KeyError));
  g_dlls.python23.AddExport("PyModule_AddObject", (unsigned long)PyModule_AddObject);
  g_dlls.python23.AddExport("_PyObject_New", (unsigned long)_PyObject_New);
  g_dlls.python23.AddExport("PyThread_release_lock", (unsigned long)PyThread_release_lock);
  g_dlls.python23.AddExport("PyThread_acquire_lock", (unsigned long)PyThread_acquire_lock);
  g_dlls.python23.AddExport("Py_FindMethod", (unsigned long)Py_FindMethod);
  g_dlls.python23.AddExport("PyModule_AddIntConstant", (unsigned long)PyModule_AddIntConstant);
  g_dlls.python23.AddExport("PyErr_NewException", (unsigned long)PyErr_NewException);
  g_dlls.python23.AddExport("PyType_Type", (unsigned long)(&PyType_Type));
  g_dlls.python23.AddExport("_PyString_Resize", (unsigned long)_PyString_Resize);
  g_dlls.python23.AddExport("PyErr_Format", (unsigned long)PyErr_Format);
  g_dlls.python23.AddExport("PyExc_MemoryError", (unsigned long)(&PyExc_MemoryError));
  g_dlls.python23.AddExport("PyEval_RestoreThread", (unsigned long)PyEval_RestoreThread);
  g_dlls.python23.AddExport("PyEval_SaveThread", (unsigned long)PyEval_SaveThread);
  g_dlls.python23.AddExport("PyThread_allocate_lock", (unsigned long)PyThread_allocate_lock);
  g_dlls.python23.AddExport("PyString_FromStringAndSize", (unsigned long)PyString_FromStringAndSize);
  g_dlls.python23.AddExport("PyUnicodeUCS2_GetSize", (unsigned long)PyUnicodeUCS2_GetSize);
  g_dlls.python23.AddExport("PyList_New", (unsigned long)PyList_New);
  g_dlls.python23.AddExport("PyDict_GetItem", (unsigned long)PyDict_GetItem);
  g_dlls.python23.AddExport("PyInt_AsLong", (unsigned long)PyInt_AsLong);
  g_dlls.python23.AddExport("PyErr_Occurred", (unsigned long)PyErr_Occurred);
  g_dlls.python23.AddExport("PyErr_SetObject", (unsigned long)PyErr_SetObject);
  g_dlls.python23.AddExport("PyObject_CallFunction", (unsigned long)PyObject_CallFunction);
  g_dlls.python23.AddExport("PyObject_SetAttrString", (unsigned long)PyObject_SetAttrString);
  g_dlls.python23.AddExport("PyErr_SetFromErrno", (unsigned long)PyErr_SetFromErrno);
  g_dlls.python23.AddExport("PyExc_IOError", (unsigned long)(&PyExc_IOError));
  g_dlls.python23.AddExport("PyErr_NoMemory", (unsigned long)PyErr_NoMemory);
  g_dlls.python23.AddExport("PyFile_AsFile", (unsigned long)PyFile_AsFile);
  g_dlls.python23.AddExport("PyErr_Clear", (unsigned long)PyErr_Clear);
  g_dlls.python23.AddExport("PyObject_GetAttrString", (unsigned long)PyObject_GetAttrString);
  g_dlls.python23.AddExport("PyType_IsSubtype", (unsigned long)PyType_IsSubtype);
  g_dlls.python23.AddExport("PyFile_Type", (unsigned long)(&PyFile_Type));
  g_dlls.python23.AddExport("PyString_AsString", (unsigned long)PyString_AsString);
  g_dlls.python23.AddExport("PyString_Type", (unsigned long)(&PyString_Type));
  g_dlls.python23.AddExport("PyDict_New", (unsigned long)PyDict_New);
  g_dlls.python23.AddExport("PyObject_GC_Track", (unsigned long)PyObject_GC_Track);
  g_dlls.python23.AddExport("PyObject_GC_Del", (unsigned long)PyObject_GC_Del);
  g_dlls.python23.AddExport("_PyObject_GC_New", (unsigned long)_PyObject_GC_New);
  g_dlls.python23.AddExport("PyObject_IsTrue", (unsigned long)PyObject_IsTrue);
  g_dlls.python23.AddExport("PyObject_GC_UnTrack", (unsigned long)PyObject_GC_UnTrack);
  g_dlls.python23.AddExport("PyList_Append", (unsigned long)PyList_Append);
  g_dlls.python23.AddExport("_Py_ZeroStruct", (unsigned long)(&_Py_ZeroStruct));
  g_dlls.python23.AddExport("_Py_TrueStruct", (unsigned long)(&_Py_TrueStruct));
  g_dlls.python23.AddExport("PyExc_AttributeError", (unsigned long)(&PyExc_AttributeError));
  g_dlls.python23.AddExport("PyExc_RuntimeError", (unsigned long)(&PyExc_RuntimeError));
  g_dlls.python23.AddExport("PyDict_Type", (unsigned long)(&PyDict_Type));
  g_dlls.python23.AddExport("PyArg_ParseTupleAndKeywords", (unsigned long)PyArg_ParseTupleAndKeywords);
  g_dlls.python23.AddExport("PyUnicodeUCS2_Decode", (unsigned long)PyUnicodeUCS2_Decode);
  g_dlls.python23.AddExport("PyModule_New", (unsigned long)PyModule_New);
  g_dlls.python23.AddExport("PyModule_GetDict", (unsigned long)PyModule_GetDict);
  g_dlls.python23.AddExport("PySys_GetObject", (unsigned long)PySys_GetObject);
  g_dlls.python23.AddExport("PyDict_SetItem", (unsigned long)PyDict_SetItem);
  g_dlls.python23.AddExport("PyErr_Fetch", (unsigned long)PyErr_Fetch);
  g_dlls.python23.AddExport("PyErr_Restore", (unsigned long)PyErr_Restore);
  g_dlls.python23.AddExport("_PyThreadState_Current", (unsigned long)(&_PyThreadState_Current));
  g_dlls.python23.AddExport("PyEval_GetGlobals", (unsigned long)PyEval_GetGlobals);
  g_dlls.python23.AddExport("PyFrame_New", (unsigned long)PyFrame_New);
  g_dlls.python23.AddExport("PyEval_CallObjectWithKeywords", (unsigned long)PyEval_CallObjectWithKeywords);
  g_dlls.python23.AddExport("PyTraceBack_Here", (unsigned long)PyTraceBack_Here);
  g_dlls.python23.AddExport("PyCode_New", (unsigned long)PyCode_New);
  g_dlls.python23.AddExport("PyUnicodeUCS2_DecodeUTF8", (unsigned long)PyUnicodeUCS2_DecodeUTF8);
  g_dlls.python23.AddExport("_Py_NoneStruct", (unsigned long)(&_Py_NoneStruct));
  g_dlls.python23.AddExport("PyObject_Call", (unsigned long)PyObject_Call);
  g_dlls.python23.AddExport("PyTuple_New", (unsigned long)PyTuple_New);
  g_dlls.python23.AddExport("PyDict_SetItemString", (unsigned long)PyDict_SetItemString);
  g_dlls.python23.AddExport("PyErr_CheckSignals", (unsigned long)PyErr_CheckSignals);
  g_dlls.python23.AddExport("PyCObject_AsVoidPtr", (unsigned long)PyCObject_AsVoidPtr);
  g_dlls.python23.AddExport("PyImport_ImportModule", (unsigned long)PyImport_ImportModule);

  g_dlls.python23.AddExport("PyObject_Size", (unsigned long)PyObject_Size);
  g_dlls.python23.AddExport("_PyUnicodeUCS2_IsLinebreak", (unsigned long)_PyUnicodeUCS2_IsLinebreak);
  g_dlls.python23.AddExport("_PyUnicodeUCS2_IsWhitespace", (unsigned long)_PyUnicodeUCS2_IsWhitespace);
  g_dlls.python23.AddExport("_PyUnicodeUCS2_IsNumeric", (unsigned long)_PyUnicodeUCS2_IsNumeric);
  g_dlls.python23.AddExport("_PyUnicodeUCS2_IsDigit", (unsigned long)_PyUnicodeUCS2_IsDigit);
  g_dlls.python23.AddExport("_PyUnicodeUCS2_IsDecimalDigit", (unsigned long)_PyUnicodeUCS2_IsDecimalDigit);
  g_dlls.python23.AddExport("_PyUnicodeUCS2_IsAlpha", (unsigned long)_PyUnicodeUCS2_IsAlpha);
  g_dlls.python23.AddExport("PyOS_CheckStack", (unsigned long)PyOS_CheckStack);
  g_dlls.python23.AddExport("PySequence_GetSlice", (unsigned long)PySequence_GetSlice);
  g_dlls.python23.AddExport("PyCallIter_New", (unsigned long)PyCallIter_New);
  g_dlls.python23.AddExport("PyObject_CallObject", (unsigned long)PyObject_CallObject);
  g_dlls.python23.AddExport("PyCallable_Check", (unsigned long)PyCallable_Check);
  g_dlls.python23.AddExport("PyImport_Import", (unsigned long)PyImport_Import);
  g_dlls.python23.AddExport("PyExc_IndexError", (unsigned long)(&PyExc_IndexError));
  g_dlls.python23.AddExport("PyObject_GetItem", (unsigned long)PyObject_GetItem);
  g_dlls.python23.AddExport("PyObject_CallMethod", (unsigned long)PyObject_CallMethod);
  g_dlls.python23.AddExport("PySequence_GetItem", (unsigned long)PySequence_GetItem);
  g_dlls.python23.AddExport("PyObject_Init", (unsigned long)PyObject_Init);
  g_dlls.python23.AddExport("_PyUnicodeUCS2_ToLowercase", (unsigned long)_PyUnicodeUCS2_ToLowercase);
  g_dlls.python23.AddExport("PyList_Type", (unsigned long)(&PyList_Type));
  g_dlls.python23.AddExport("PyObject_Malloc", (unsigned long)PyObject_Malloc);
  g_dlls.python23.AddExport("PyObject_InitVar", (unsigned long)PyObject_InitVar);
  g_dlls.python23.AddExport("PyInt_Type", (unsigned long)(&PyInt_Type));
  g_dlls.python23.AddExport("PyLong_AsUnsignedLong", (unsigned long)PyLong_AsUnsignedLong);

  g_dlls.python23.AddExport("PyMem_Free", (unsigned long)PyMem_Free);
  g_dlls.python23.AddExport("PyTuple_Type", (unsigned long)(&PyTuple_Type));
  g_dlls.python23.AddExport("PyLong_Type", (unsigned long)(&PyLong_Type));
  g_dlls.python23.AddExport("PyExc_ImportError", (unsigned long)(&PyExc_ImportError));
  g_dlls.python23.AddExport("Py_AtExit", (unsigned long)Py_AtExit);
  g_dlls.python23.AddExport("PyFloat_AsDouble", (unsigned long)PyFloat_AsDouble);
  g_dlls.python23.AddExport("PyType_GenericNew", (unsigned long)PyType_GenericNew);
  g_dlls.python23.AddExport("PyObject_GenericGetAttr", (unsigned long)PyObject_GenericGetAttr);
  g_dlls.python23.AddExport("PyType_GenericAlloc", (unsigned long)PyType_GenericAlloc);

  g_dlls.python23.AddExport("PyNumber_Check", (unsigned long)PyNumber_Check);
  g_dlls.python23.AddExport("PyExc_OverflowError", (unsigned long)(&PyExc_OverflowError));
  g_dlls.python23.AddExport("PyObject_AsFileDescriptor", (unsigned long)PyObject_AsFileDescriptor);
  g_dlls.python23.AddExport("PyList_GetItem", (unsigned long)PyList_GetItem);
  g_dlls.python23.AddExport("PyList_Size", (unsigned long)PyList_Size);
  g_dlls.python23.AddExport("PyList_SetItem", (unsigned long)PyList_SetItem);
  g_dlls.python23.AddExport("PyErr_SetExcFromWindowsErr", (unsigned long)PyErr_SetExcFromWindowsErr);

  g_dlls.python23.AddExport("PyString_Size", (unsigned long)PyString_Size);
  g_dlls.python23.AddExport("PyDict_Next", (unsigned long)PyDict_Next);
  g_dlls.python23.AddExport("PyObject_SetAttr", (unsigned long)PyObject_SetAttr);
  g_dlls.python23.AddExport("PyObject_GetAttr", (unsigned long)PyObject_GetAttr);
  g_dlls.python23.AddExport("PyObject_Dir", (unsigned long)PyObject_Dir);
  g_dlls.python23.AddExport("PyInstance_Type", (unsigned long)(&PyInstance_Type));
  g_dlls.python23.AddExport("PyClass_Type", (unsigned long)(&PyClass_Type));
  g_dlls.python23.AddExport("PyDict_DelItem", (unsigned long)PyDict_DelItem);
  g_dlls.python23.AddExport("PyIter_Next", (unsigned long)PyIter_Next);
  g_dlls.python23.AddExport("PyMem_Realloc", (unsigned long)PyMem_Realloc);
  g_dlls.python23.AddExport("PyMem_Malloc", (unsigned long)PyMem_Malloc);
  g_dlls.python23.AddExport("PyObject_GetIter", (unsigned long)PyObject_GetIter);
  g_dlls.python23.AddExport("PyObject_Str", (unsigned long)PyObject_Str);
  g_dlls.python23.AddExport("PyNumber_Float", (unsigned long)PyNumber_Float);
  g_dlls.python23.AddExport("PySequence_Size", (unsigned long)PySequence_Size);
  g_dlls.python23.AddExport("PySequence_Check", (unsigned long)PySequence_Check);
  g_dlls.python23.AddExport("PyDict_Keys", (unsigned long)PyDict_Keys);
  g_dlls.python23.AddExport("PyType_Ready", (unsigned long)PyType_Ready);
  g_dlls.python23.AddExport("PyErr_BadArgument", (unsigned long)PyErr_BadArgument);

  g_dlls.python23.AddExport("PyList_GetSlice", (unsigned long)PyList_GetSlice);
  g_dlls.python23.AddExport("PyObject_AsCharBuffer", (unsigned long)PyObject_AsCharBuffer);
  g_dlls.python23.AddExport("PyThread_free_lock", (unsigned long)PyThread_free_lock);
  g_dlls.python23.AddExport("PyString_Concat", (unsigned long)PyString_Concat);
  g_dlls.python23.AddExport("PyExc_SystemError", (unsigned long)(&PyExc_SystemError));
  g_dlls.python23.AddExport("PyExc_EOFError", (unsigned long)(&PyExc_EOFError));
  g_dlls.python23.AddExport("PyObject_GenericSetAttr", (unsigned long)PyObject_GenericSetAttr);

  g_dlls.python23.AddExport("PyNumber_Multiply", (unsigned long)PyNumber_Multiply);
  g_dlls.python23.AddExport("PyNumber_Add", (unsigned long)PyNumber_Add);
  g_dlls.python23.AddExport("PyLong_AsLong", (unsigned long)PyLong_AsLong);
  g_dlls.python23.AddExport("PyTuple_GetItem", (unsigned long)PyTuple_GetItem);
  g_dlls.python23.AddExport("PyNumber_Divmod", (unsigned long)PyNumber_Divmod);
  g_dlls.python23.AddExport("PyNumber_FloorDivide", (unsigned long)PyNumber_FloorDivide);
  g_dlls.python23.AddExport("PyLong_FromLong", (unsigned long)PyLong_FromLong);
  g_dlls.python23.AddExport("PyLong_AsDouble", (unsigned long)PyLong_AsDouble);
  g_dlls.python23.AddExport("PyLong_FromDouble", (unsigned long)PyLong_FromDouble);
  g_dlls.python23.AddExport("PyFloat_Type", (unsigned long)(&PyFloat_Type));
  g_dlls.python23.AddExport("PyString_FromFormat", (unsigned long)PyString_FromFormat);
  g_dlls.python23.AddExport("PyObject_HasAttrString", (unsigned long)PyObject_HasAttrString);
  g_dlls.python23.AddExport("PyExc_NotImplementedError", (unsigned long)(&PyExc_NotImplementedError));
  g_dlls.python23.AddExport("PyDict_Size", (unsigned long)PyDict_Size);
  g_dlls.python23.AddExport("_PyObject_GetDictPtr", (unsigned long)_PyObject_GetDictPtr);
  g_dlls.python23.AddExport("PyObject_Repr", (unsigned long)PyObject_Repr);
  g_dlls.python23.AddExport("PyString_ConcatAndDel", (unsigned long)PyString_ConcatAndDel);
  g_dlls.python23.AddExport("PyErr_ExceptionMatches", (unsigned long)PyErr_ExceptionMatches);
  g_dlls.python23.AddExport("PyObject_Hash", (unsigned long)PyObject_Hash);
  g_dlls.python23.AddExport("PyNumber_Negative", (unsigned long)PyNumber_Negative);
  g_dlls.python23.AddExport("_Py_NotImplementedStruct", (unsigned long)(&_Py_NotImplementedStruct));
  g_dlls.python23.AddExport("PySymtable_Free", (unsigned long)PySymtable_Free);
  g_dlls.python23.AddExport("Py_SymtableString", (unsigned long)Py_SymtableString);

  g_dlls.python23.AddExport("PyNode_Compile", (unsigned long)PyNode_Compile);
  g_dlls.python23.AddExport("PyParser_SimpleParseString", (unsigned long)PyParser_SimpleParseString);
  g_dlls.python23.AddExport("PyNode_New", (unsigned long)PyNode_New);
  g_dlls.python23.AddExport("PyNode_AddChild", (unsigned long)PyNode_AddChild);
  g_dlls.python23.AddExport("PyTuple_SetItem", (unsigned long)PyTuple_SetItem);
  g_dlls.python23.AddExport("PyNode_Free", (unsigned long)PyNode_Free);
  g_dlls.python23.AddExport("PyExc_EnvironmentError", (unsigned long)(&PyExc_EnvironmentError));
  g_dlls.python23.AddExport("PyErr_SetFromWindowsErr", (unsigned long)PyErr_SetFromWindowsErr);
  g_dlls.python23.AddExport("PyRun_String", (unsigned long)PyRun_String);
  g_dlls.python23.AddExport("PyDict_GetItemString", (unsigned long)PyDict_GetItemString);
  g_dlls.python23.AddExport("PyDict_DelItemString", (unsigned long)PyDict_DelItemString);
  g_dlls.python23.AddExport("Py_FatalError", (unsigned long)Py_FatalError);
  g_dlls.python23.AddExport("PyGILState_Ensure", (unsigned long)PyGILState_Ensure);
  g_dlls.python23.AddExport("PyErr_Print", (unsigned long)PyErr_Print);
  g_dlls.python23.AddExport("PyString_AsStringAndSize", (unsigned long)PyString_AsStringAndSize);
  g_dlls.python23.AddExport("PyGILState_Release", (unsigned long)PyGILState_Release);
  g_dlls.python23.AddExport("PyEval_InitThreads", (unsigned long)PyEval_InitThreads);
  g_dlls.python23.AddExport("PyArg_Parse", (unsigned long)PyArg_Parse);
  g_dlls.python23.AddExport("PyExc_RuntimeWarning", (unsigned long)(&PyExc_RuntimeWarning));
  g_dlls.python23.AddExport("PyErr_Warn", (unsigned long)PyErr_Warn);
}
