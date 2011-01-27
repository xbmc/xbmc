#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif
#if (defined USE_EXTERNAL_PYTHON)
  #if (defined HAVE_LIBPYTHON2_6)
    #include <python2.6/pyconfig.h>
  #elif (defined HAVE_LIBPYTHON2_5)
    #include <python2.5/pyconfig.h>
  #elif (defined HAVE_LIBPYTHON2_4)
    #include <python2.4/pyconfig.h>
  #else
    #error "Could not determine version of Python to use."
  #endif
#else
  #include "python/Include/pyconfig.h"
#endif
#ifndef __arm__
#ifndef __APPLE__
#if defined(__PPC__)
#define FUNCTION(name)        \
  .globl name;                \
  .type name, @function;      \
  name:                       \
    lis 11,(p_##name)@ha;     \
    lwz 12,(p_##name)@l(11);  \
    mtctr 12;                 \
    bctr
#elif defined(__arm__)
#define FUNCTION(name)          \
    .globl name;            \
    .type name, %function;      \
    name:               \
        b (p_##name)
#else
#define FUNCTION(name)        \
  .globl name;                \
  .type name, @function;      \
  name:                       \
    jmp *(p_##name)
#endif
#elif defined(__POWERPC__)
#define FUNCTION(name)             \
  .globl _##name                  @\
  _##name:                        @\
    addis r11,0,ha16(_p_##name)   @\
    lwz r12,lo16(_p_##name)(r11)  @\
    mtctr r12                     @\
    bctr
#else
#define FUNCTION(name)        \
  .globl _##name;             \
  _##name:                    \
    jmp *(_p_##name)
#endif

.text

  FUNCTION(PyEval_ReleaseLock)
  FUNCTION(PyEval_AcquireLock)
  FUNCTION(PyThreadState_Get)
  FUNCTION(PyRun_SimpleString)
  FUNCTION(PyRun_SimpleStringFlags)
  FUNCTION(PyRun_SimpleFileExFlags)
  FUNCTION(PyEval_InitThreads)
  FUNCTION(PyEval_ThreadsInitialized)
  FUNCTION(Py_Initialize)
  FUNCTION(Py_IsInitialized)
  FUNCTION(Py_Finalize)
  FUNCTION(Py_NewInterpreter)
  FUNCTION(Py_EndInterpreter)
  FUNCTION(PyThreadState_Swap)
  FUNCTION(PyErr_SetString)
  FUNCTION(PyThreadState_New)
  FUNCTION(PyErr_Print)
  FUNCTION(PyErr_Occurred)
  FUNCTION(PyRun_String)
  FUNCTION(PyRun_File)
  FUNCTION(PyImport_AddModule)
  FUNCTION(PyObject_Str)
  FUNCTION(PyRun_SimpleFile)
  FUNCTION(PySys_SetPath)
  FUNCTION(PyErr_Fetch)
  FUNCTION(PyImport_ImportModule)
#ifdef _LINUX
  FUNCTION(Py_GetPath)
#endif
  FUNCTION(PyThreadState_Delete)
  FUNCTION(PyThreadState_Clear)
  FUNCTION(Py_BuildValue)
  FUNCTION(PyArg_Parse)
  FUNCTION(PyArg_ParseTuple)
  FUNCTION(PyType_IsSubtype)
  FUNCTION(PyArg_ParseTupleAndKeywords)
  FUNCTION(PyString_AsString)
  FUNCTION(Py_AddPendingCall)
  FUNCTION(PyList_GetItem)
  FUNCTION(PyList_Size)
  FUNCTION(PyList_New)
  FUNCTION(PyList_Append)
  FUNCTION(_PyObject_New)
  FUNCTION(PyLong_AsLong)
  FUNCTION(PyLong_AsLongLong)
  FUNCTION(PyErr_Format)
#if Py_UNICODE_SIZE == 2
  FUNCTION(PyUnicodeUCS2_AsUTF8String)
  FUNCTION(PyUnicodeUCS2_DecodeUTF8)
#else
  FUNCTION(PyUnicodeUCS4_AsUTF8String)
  FUNCTION(PyUnicodeUCS4_DecodeUTF8)
#endif
  FUNCTION(Py_MakePendingCalls)
  FUNCTION(PyEval_SaveThread)
  FUNCTION(PyEval_RestoreThread)
  FUNCTION(PyLong_FromLong)
  FUNCTION(PyModule_AddStringConstant)
  FUNCTION(PyModule_AddObject)
#if (defined USE_EXTERNAL_PYTHON) && (!defined HAVE_LIBPYTHON2_4)
  /* Upstream Python rename Py_InitModule4 for 64-bit systems, for Python
   versions higher than 2.4 */
  #if SIZEOF_SIZE_T != SIZEOF_INT
  FUNCTION(Py_InitModule4_64)
  #else
  FUNCTION(Py_InitModule4)
  #endif
#else
  FUNCTION(Py_InitModule4)
#endif
  FUNCTION(PyInt_AsLong)
  FUNCTION(PyFloat_AsDouble)
  FUNCTION(PyString_FromString)
  FUNCTION(PyBool_FromLong)
  FUNCTION(PyModule_AddIntConstant)
  FUNCTION(PyObject_CallFunction)
  FUNCTION(PyObject_CallMethod)
  FUNCTION(PyDict_SetItemString)
  FUNCTION(PyDict_New)
  FUNCTION(PyModule_GetDict)
  FUNCTION(PyImport_Import)
  FUNCTION(PyInt_FromLong)
  FUNCTION(PyDict_GetItemString)
  FUNCTION(PyDict_Next)
  FUNCTION(PyDict_Size)
  FUNCTION(PyTuple_New)
  FUNCTION(PyTuple_SetItem)
  FUNCTION(PyType_Ready)
  FUNCTION(PyType_GenericNew)
  FUNCTION(PySys_SetArgv)
  FUNCTION(PyObject_RichCompare)
  FUNCTION(PyFloat_FromDouble)
  FUNCTION(PyRun_FileExFlags)
  FUNCTION(PyRun_StringFlags)
  FUNCTION(PyErr_Clear)
  FUNCTION(PyErr_ExceptionMatches)
  FUNCTION(PyObject_SetAttrString)
#endif
#if defined(__linux__) && defined(__ELF__)
.section .note.GNU-stack,"",%progbits
#endif
