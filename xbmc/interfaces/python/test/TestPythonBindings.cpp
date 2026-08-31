/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

// python.h should always be included first before any other includes
#include <Python.h>

#include "interfaces/python/AddonPythonInvoker.h"

#include <gtest/gtest.h>

namespace
{

// on failure the traceback goes to stderr
bool RunPy(const char* code)
{
  return PyRun_SimpleString(code) == 0;
}

class TestPythonBindings : public ::testing::Test
{
protected:
  static void SetUpTestSuite()
  {
    CAddonPythonInvoker::GlobalInitializeModules();
    Py_Initialize();
    // first import must run in the main interpreter: the binding types are created once per process and the main interpreter is the one never destroyed
    s_mainImportOk = RunPy("import xbmc, xbmcgui, xbmcplugin, xbmcaddon, xbmcvfs, xbmcdrm\n");
  }

  // no TearDownTestSuite: Py_Finalize would destroy the process-wide binding types
  static bool s_mainImportOk;
};

bool TestPythonBindings::s_mainImportOk = false;

TEST_F(TestPythonBindings, ImportModules)
{
  EXPECT_TRUE(s_mainImportOk);
}

// there is no window system in this environment, so every object a test calls methods on must be offscreen
TEST_F(TestPythonBindings, DirectConstructWithKwargs)
{
  ASSERT_TRUE(s_mainImportOk);
  EXPECT_TRUE(RunPy(R"py(
import xbmcgui
li = xbmcgui.ListItem('direct', offscreen=True)
assert li.getLabel() == 'direct', li.getLabel()
)py"));
}

// construction happens in tp_new, which drops keyword arguments when retrying for a subclass, so offscreen must be positional here
TEST_F(TestPythonBindings, SubclassExtraKwargs)
{
  ASSERT_TRUE(s_mainImportOk);
  EXPECT_TRUE(RunPy(R"py(
import xbmcgui
class Item(xbmcgui.ListItem):
    def __init__(self, label, label2, path, offscreen, extra=None):
        super().__init__(label, label2, path, offscreen)
        self.extra = extra
li = Item('sub', '', '', True, extra=7)
assert li.getLabel() == 'sub', li.getLabel()
assert li.extra == 7
)py"));
}

// the service addon idiom: an __init__ that never calls super() must still yield a constructed C++ object
TEST_F(TestPythonBindings, SubclassNeverCallsSuper)
{
  ASSERT_TRUE(s_mainImportOk);
  EXPECT_TRUE(RunPy(R"py(
import xbmcgui
class A(xbmcgui.Action):
    def __init__(self):
        pass
a = A()
assert a.getId() == -1, a.getId()
)py"));
}

// an xbmc type passed through an xbmcgui-obtained object crosses the shared type table
TEST_F(TestPythonBindings, CrossModule)
{
  ASSERT_TRUE(s_mainImportOk);
  EXPECT_TRUE(RunPy(R"py(
import xbmc, xbmcgui
li = xbmcgui.ListItem('cast', '', '', True)
tag = li.getVideoInfoTag()
assert isinstance(tag, xbmc.InfoTagVideo)
tag.setCast([xbmc.Actor('a', 'lead'), xbmc.Actor('b')])
)py"));
}

// PyType_Ready mirrors tp_init into the class dict as a wrapper_descriptor; autodoc's constructor docstring injection replaces it with a method_descriptor that runs a second construction
TEST_F(TestPythonBindings, NoInitInjection)
{
  ASSERT_TRUE(s_mainImportOk);
  EXPECT_TRUE(RunPy(R"py(
import xbmc, xbmcaddon, xbmcdrm, xbmcgui, xbmcvfs
for cls in (xbmcgui.ListItem, xbmcgui.Action, xbmcgui.Window, xbmcgui.Dialog,
            xbmc.Actor, xbmcvfs.File, xbmcvfs.Stat, xbmcaddon.Addon,
            xbmcdrm.CryptoSession):
    kind = type(vars(cls)['__init__']).__name__
    assert kind == 'wrapper_descriptor', (cls, kind)
assert xbmcgui.ListItem.setLabel.__doc__
)py"));
}

// two live sub-interpreters: unpatched SWIG re-creates the builtin types per interpreter and repoints the process-wide clientdata (swig/swig#3535)
TEST_F(TestPythonBindings, TwoSubInterpreters)
{
  ASSERT_TRUE(s_mainImportOk);
  PyThreadState* mainState = PyThreadState_Get();

  PyThreadState* first = Py_NewInterpreter();
  ASSERT_NE(first, nullptr);
  EXPECT_TRUE(RunPy(R"py(
import xbmcgui
li = xbmcgui.ListItem('first', '', '', True)
assert li.getLabel() == 'first'
)py"));

  PyThreadState_Swap(mainState);
  PyThreadState* second = Py_NewInterpreter();
  ASSERT_NE(second, nullptr);
  EXPECT_TRUE(RunPy(R"py(
import xbmcgui
li = xbmcgui.ListItem('second', '', '', True)
assert li.getLabel() == 'second'
del li
)py"));
  Py_EndInterpreter(second);

  PyThreadState_Swap(first);
  EXPECT_TRUE(RunPy(R"py(
li2 = xbmcgui.ListItem('again', '', '', True)
assert li2.getLabel() == 'again'
assert type(li2) is type(li)
del li, li2
)py"));
  Py_EndInterpreter(first);

  PyThreadState_Swap(mainState);
  EXPECT_TRUE(RunPy(R"py(
import xbmcgui
assert xbmcgui.ListItem('main', '', '', True).getLabel() == 'main'
)py"));
}

} // namespace
