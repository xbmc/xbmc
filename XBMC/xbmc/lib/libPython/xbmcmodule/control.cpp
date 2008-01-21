#include "stdafx.h"
#include "control.h"
#include "pyutil.h"
#include "../../../utils/GUIInfoManager.h"
#include "../guilib/GuiControlFactory.h"

#ifndef __GNUC__
#pragma code_seg("PY_TEXT")
#pragma data_seg("PY_DATA")
#pragma bss_seg("PY_BSS")
#pragma const_seg("PY_RDATA")
#endif

#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{
  /*
  // not used for now

  PyObject* Control_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
  {
    Control *self;

    self = (Control*)type->tp_alloc(type, 0);
    if (!self) return NULL;

    self->iControlId = 0;
    self->iParentId = 0;
    self->pGUIControl = NULL;

    return (PyObject*)self;
  }

  void Control_Dealloc(Control* self)
  {
    self->ob_type->tp_free((PyObject*)self);
  }
*/

  /* This function should return -1 if obj1 is less than obj2,
   * 0 if they are equal, and 1 if obj1 is greater than obj2
   */
  int Control_Compare(PyObject* obj1, PyObject* obj2)
  {
    if(((Control*)obj1)->iControlId < ((Control*)obj2)->iControlId) return -1;
    if(((Control*)obj1)->iControlId > ((Control*)obj2)->iControlId) return 1;
    return 0;
  }

  // getId() Method
  PyDoc_STRVAR(getId__doc__,
    "getId() -- Returns the control's current id as an integer.\n"
    "\n"
    "example:\n"
    "  - id = self.button.getId()\n");

  PyObject* Control_GetId(Control* self, PyObject* args)
  {
    return Py_BuildValue("i", self->iControlId);
  }

  // getPosition() Method
  PyDoc_STRVAR(getPosition__doc__,
    "getPosition() -- Returns the control's current position as a x,y integer tuple.\n"
    "\n"
    "example:\n"
    "  - pos = self.button.getPosition()\n");

  PyObject* Control_GetPosition(Control* self, PyObject* args)
  {
    return Py_BuildValue("(ll)", self->dwPosX, self->dwPosY);
  }

  // getHeight() Method
  PyDoc_STRVAR(getHeight__doc__,
    "getHeight() -- Returns the control's current height as an integer.\n"
    "\n"
    "example:\n"
    "  - height = self.button.getHeight()\n");

  PyObject* Control_GetHeight(Control* self, PyObject* args)
  {
    return Py_BuildValue("l", self->dwHeight);
  }

  // getWidth() Method
  PyDoc_STRVAR(getWidth__doc__,
    "getWidth() -- Returns the control's current width as an integer.\n"
    "\n"
    "example:\n"
    "  - width = self.button.getWidth()\n");

  PyObject* Control_GetWidth(Control* self, PyObject* args)
  {
    return Py_BuildValue("l", self->dwWidth);
  }

  // setEnabled() Method
  PyDoc_STRVAR(setEnabled__doc__,
    "setEnabled(enabled) -- Set's the control's enabled/disabled state.\n"
    "\n"
    "enabled        : bool - True=enabled / False=disabled.\n"
    "\n"
    "example:\n"
    "  - self.button.setEnabled(False)\n");

  PyObject* Control_SetEnabled(Control* self, PyObject* args)
  {
    bool enabled;
    if (!PyArg_ParseTuple(args, "b", &enabled)) return NULL;

    PyGUILock();
    if (self->pGUIControl)   self->pGUIControl->SetEnabled(enabled);
    PyGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  // setVisible() Method
  PyDoc_STRVAR(setVisible__doc__,
    "setVisible(visible) -- Set's the control's visible/hidden state.\n"
    "\n"
    "visible        : bool - True=visible / False=hidden.\n"
    "\n"
    "example:\n"
    "  - self.button.setVisible(False)\n");

  PyObject* Control_SetVisible(Control* self, PyObject* args)
  {
    bool visible;
    if (!PyArg_ParseTuple(args, "b", &visible)) return NULL;

    PyGUILock();
    if (self->pGUIControl)
    {
      self->pGUIControl->SetVisible(visible);
    }
    PyGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  // setVisibleCondition() Method
  PyDoc_STRVAR(setVisibleCondition__doc__,
    "setVisibleCondition(visible[,allowHiddenFocus]) -- Set's the control's visible condition.\n"
    "    Allows XBMC to control the visible status of the control.\n"
    "\n"
    "visible          : string - Visible condition.\n"
    "allowHiddenFocus : bool - True=gains focus even if hidden.\n"
    "\n"
    "List of Conditions - http://www.xboxmediacenter.com/wiki/index.php?title=List_of_Boolean_Conditions \n"
    "\n"
    "example:\n"
    "  - self.button.setVisibleCondition('[System.KaiEnabled + !Skin.String(KAI)]', True)\n");

  PyObject* Control_SetVisibleCondition(Control* self, PyObject* args)
  {
    char *cVisible = NULL;
    bool bHidden = false;

    if (!PyArg_ParseTuple(args, "s|b", &cVisible, &bHidden)) return NULL;

    int ret = g_infoManager.TranslateString(cVisible);

    PyGUILock();
    if (self->pGUIControl)
      self->pGUIControl->SetVisibleCondition(ret, bHidden);
    PyGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  // setEnableCondition() Method
  PyDoc_STRVAR(setEnableCondition__doc__,
    "setEnableCondition(enable) -- Set's the control's enabled condition.\n"
    "    Allows XBMC to control the enabled status of the control.\n"
    "\n"
    "enable           : string - Enable condition.\n"
    "\n"
    "List of Conditions - http://www.xboxmediacenter.com/wiki/index.php?title=List_of_Boolean_Conditions \n"
    "\n"
    "example:\n"
    "  - self.button.setEnableCondition('System.InternetState')\n");

  PyObject* Control_SetEnableCondition(Control* self, PyObject* args)
  {
    char *cEnable = NULL;

    if (!PyArg_ParseTuple(args, "s", &cEnable)) return NULL;

    int ret = g_infoManager.TranslateString(cEnable);

    PyGUILock();
    if (self->pGUIControl)
      self->pGUIControl->SetEnableCondition(ret);
    PyGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  // setAnimations() Method
  PyDoc_STRVAR(setAnimations__doc__,
    "setAnimations([(event, attr,)*]) -- Set's the control's animations.\n"
    "\n"
    "[(event,attr,)*] : list - A list of tuples consisting of event and attributes pairs.\n"
    "  - event        : string - The event to animate.\n"
    "  - attr         : string - The whole attribute string separated by spaces.\n"
    "\n"
    "Animating your skin - http://www.xboxmediacenter.com/wiki/index.php?title=Animating_Your_Skin \n"
    "\n"
    "example:\n"
    "  - self.button.setAnimations([('focus', 'effect=zoom end=90,247,220,56 time=0')])\n");

  PyObject* Control_SetAnimations(Control* self, PyObject* args)
  {
    PyObject *pList = NULL;
    if (!PyArg_ParseTuple(args, "O", &pList) || pList == NULL || !PyObject_TypeCheck(pList, &PyList_Type))
    {
      PyErr_SetString(PyExc_TypeError, "Object should be of type List");
      return NULL;
    }

    TiXmlDocument xmlDoc;
    TiXmlElement xmlRootElement("control");
    TiXmlNode *pRoot = xmlDoc.InsertEndChild(xmlRootElement);
    if (!pRoot)
    {
      PyErr_SetString(PyExc_TypeError, "TiXmlNode creation error");
      return NULL;
    }
    vector<CAnimation> animations;
    for (int anim = 0; anim < PyList_Size(pList); anim++)
    {
      PyObject *pTuple = NULL;
      char *cEvent = NULL;
      char *cAttr = NULL;
      pTuple = PyList_GetItem(pList, anim);
      if (pTuple == NULL || !PyObject_TypeCheck(pTuple, &PyTuple_Type))
      {
        PyErr_SetString(PyExc_TypeError, "List must only contain tuples");
        return NULL;
      }
      if (!PyArg_ParseTuple(pTuple, "ss", &cEvent, &cAttr))
      {
        PyErr_SetString(PyExc_TypeError, "Error unpacking tuple found in list");
        return NULL;
      }

      if (NULL != cAttr && NULL != cEvent)
      {
        TiXmlElement pNode("animation");
        CStdStringArray attrs;
        StringUtils::SplitString(cAttr, " ", attrs);
        for (unsigned int i = 0; i < attrs.size(); i++)
        {
          CStdStringArray attrs2;
          StringUtils::SplitString(attrs[i], "=", attrs2);
          if (attrs2.size() == 2)
            pNode.SetAttribute(attrs2[0], attrs2[1]);
        }
        TiXmlText value(cEvent);
        pNode.InsertEndChild(value);
        pRoot->InsertEndChild(pNode);
      }
    }

    //bool ret = xmlDoc.SaveFile("q:\\userdata\\test.txt");

    const FRECT animRect = { (float)self->dwPosX, (float)self->dwPosY, (float)self->dwWidth, (float)self->dwHeight };
    PyGUILock();
    if (self->pGUIControl)
      CGUIControlFactory::GetAnimations(pRoot, animRect, animations);
    self->pGUIControl->SetAnimations(animations);
    PyGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  // setPosition() Method
  PyDoc_STRVAR(setPosition__doc__,
    "setPosition(x, y) -- Set's the controls position.\n"
    "\n"
    "x              : integer - x coordinate of control.\n"
    "y              : integer - y coordinate of control.\n"
    "\n"
    "*Note, You may use negative integers. (e.g sliding a control into view)\n"
    "\n"
    "example:\n"
    "  - self.button.setPosition(100, 250)\n");

  PyObject* Control_SetPosition(Control* self, PyObject* args)
  {
    if (!PyArg_ParseTuple(args, "ll", &self->dwPosX, &self->dwPosY)) return NULL;

    PyGUILock();
    if (self->pGUIControl) self->pGUIControl->SetPosition((float)self->dwPosX, (float)self->dwPosY);
    PyGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  // setWidth() Method
  PyDoc_STRVAR(setWidth__doc__,
    "setWidth(width) -- Set's the controls width.\n"
    "\n"
    "width          : integer - width of control.\n"
    "\n"
    "example:\n"
    "  - self.image.setWidth(100)\n");

  PyObject* Control_SetWidth(Control* self, PyObject* args)
  {
    if (!PyArg_ParseTuple(args, "l", &self->dwWidth)) return NULL;

    PyGUILock();
    if (self->pGUIControl) self->pGUIControl->SetWidth((float)self->dwWidth);
    PyGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  // setHeight() Method
  PyDoc_STRVAR(setHeight__doc__,
    "setHeight(height) -- Set's the controls height.\n"
    "\n"
    "height         : integer - height of control.\n"
    "\n"
    "example:\n"
    "  - self.image.setHeight(100)\n");

  PyObject* Control_SetHeight(Control* self, PyObject* args)
  {
    if (!PyArg_ParseTuple(args, "l", &self->dwHeight)) return NULL;

    PyGUILock();
    if (self->pGUIControl) self->pGUIControl->SetHeight((float)self->dwHeight);
    PyGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  // setNavigation() Method
  PyDoc_STRVAR(setNavigation__doc__,
    "setNavigation(up, down, left, right) -- Set's the controls navigation.\n"
    "\n"
    "up             : control object - control to navigate to on up.\n"
    "down           : control object - control to navigate to on down.\n"
    "left           : control object - control to navigate to on left.\n"
    "right          : control object - control to navigate to on right.\n"
    "\n"
    "*Note, Same as controlUp(), controlDown(), controlLeft(), controlRight().\n"
    "       Set to self to disable navigation for that direction.\n"
    "\n"
    "Throws: TypeError, if one of the supplied arguments is not a control type.\n"
    "        ReferenceError, if one of the controls is not added to a window.\n"
    "\n"
    "example:\n"
    "  - self.button.setNavigation(self.button1, self.button2, self.button3, self.button4)\n");

  PyObject* Control_SetNavigation(Control* self, PyObject* args)
  {
    Control* pUpControl = NULL;
    Control* pDownControl = NULL;
    Control* pLeftControl = NULL;
    Control* pRightControl = NULL;

    if (!PyArg_ParseTuple(args, "OOOO", &pUpControl, &pDownControl, &pLeftControl, &pRightControl)) return NULL;

    // type checking, objects should be of type Control
    if(!(Control_Check(pUpControl) &&  Control_Check(pDownControl) &&
        Control_Check(pLeftControl) && Control_Check(pRightControl)))
    {
      PyErr_SetString(PyExc_TypeError, "Objects should be of type Control");
      return NULL;
    }
    if(self->iControlId == 0)
    {
      PyErr_SetString(PyExc_ReferenceError, "Control has to be added to a window first");
      return NULL;
    }

    self->iControlUp = pUpControl->iControlId;
    self->iControlDown = pDownControl->iControlId;
    self->iControlLeft = pLeftControl->iControlId;
    self->iControlRight = pRightControl->iControlId;

    PyGUILock();
    if (self->pGUIControl) self->pGUIControl->SetNavigation(
        self->iControlUp,self->iControlDown,self->iControlLeft,self->iControlRight);
    PyGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  // controlUp() Method
  PyDoc_STRVAR(controlUp__doc__,
    "controlUp(control) -- Set's the controls up navigation.\n"
    "\n"
    "control        : control object - control to navigate to on up.\n"
    "\n"
    "*Note, You can also use setNavigation(). Set to self to disable navigation.\n"
    "\n"
    "Throws: TypeError, if one of the supplied arguments is not a control type.\n"
    "        ReferenceError, if one of the controls is not added to a window.\n"
    "\n"
    "example:\n"
    "  - self.button.controlUp(self.button1)\n");

  PyObject* Control_ControlUp(Control* self, PyObject* args)
  {
    Control* pControl;
    if (!PyArg_ParseTuple(args, "O", &pControl)) return NULL;
    // type checking, object should be of type Control
    if(!Control_Check(pControl))
    {
      PyErr_SetString(PyExc_TypeError, "Object should be of type Control");
      return NULL;
    }
    if(self->iControlId == 0)
    {
      PyErr_SetString(PyExc_ReferenceError, "Control has to be added to a window first");
      return NULL;
    }

    self->iControlUp = pControl->iControlId;
    PyGUILock();
    if (self->pGUIControl) self->pGUIControl->SetNavigation(
        self->iControlUp,self->iControlDown,self->iControlLeft,self->iControlRight);
    PyGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  // controlDown() Method
  PyDoc_STRVAR(controlDown__doc__,
    "controlDown(control) -- Set's the controls down navigation.\n"
    "\n"
    "control        : control object - control to navigate to on down.\n"
    "\n"
    "*Note, You can also use setNavigation(). Set to self to disable navigation.\n"
    "\n"
    "Throws: TypeError, if one of the supplied arguments is not a control type.\n"
    "        ReferenceError, if one of the controls is not added to a window.\n"
    "\n"
    "example:\n"
    "  - self.button.controlDown(self.button1)\n");

  PyObject* Control_ControlDown(Control* self, PyObject* args)
  {
    Control* pControl;
    if (!PyArg_ParseTuple(args, "O", &pControl)) return NULL;
    // type checking, object should be of type Control
    if (!Control_Check(pControl))
    {
      PyErr_SetString(PyExc_TypeError, "Object should be of type Control");
      return NULL;
    }
    if(self->iControlId == 0)
    {
      PyErr_SetString(PyExc_ReferenceError, "Control has to be added to a window first");
      return NULL;
    }

    self->iControlDown = pControl->iControlId;
    PyGUILock();
    if (self->pGUIControl) self->pGUIControl->SetNavigation(
      self->iControlUp,self->iControlDown,self->iControlLeft,self->iControlRight);
    PyGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  // controlLeft() Method
  PyDoc_STRVAR(controlLeft__doc__,
    "controlLeft(control) -- Set's the controls left navigation.\n"
    "\n"
    "control        : control object - control to navigate to on left.\n"
    "\n"
    "*Note, You can also use setNavigation(). Set to self to disable navigation.\n"
    "\n"
    "Throws: TypeError, if one of the supplied arguments is not a control type.\n"
    "        ReferenceError, if one of the controls is not added to a window.\n"
    "\n"
    "example:\n"
    "  - self.button.controlLeft(self.button1)\n");

  PyObject* Control_ControlLeft(Control* self, PyObject* args)
  {
    Control* pControl;
    if (!PyArg_ParseTuple(args, "O", &pControl)) return NULL;
    // type checking, object should be of type Control
    if (!Control_Check(pControl))
    {
      PyErr_SetString(PyExc_TypeError, "Object should be of type Control");
      return NULL;
    }
    if(self->iControlId == 0)
    {
      PyErr_SetString(PyExc_ReferenceError, "Control has to be added to a window first");
      return NULL;
    }

    self->iControlLeft = pControl->iControlId;
    PyGUILock();
    if (self->pGUIControl)
    {
      self->pGUIControl->SetNavigation(self->iControlUp, self->iControlDown,
        self->iControlLeft,self->iControlRight);
    }
    PyGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  // controlRight() Method
  PyDoc_STRVAR(controlRight__doc__,
    "controlRight(control) -- Set's the controls right navigation.\n"
    "\n"
    "control        : control object - control to navigate to on right.\n"
    "\n"
    "*Note, You can also use setNavigation(). Set to self to disable navigation.\n"
    "\n"
    "Throws: TypeError, if one of the supplied arguments is not a control type.\n"
    "        ReferenceError, if one of the controls is not added to a window.\n"
    "\n"
    "example:\n"
    "  - self.button.controlRight(self.button1)\n");

  PyObject* Control_ControlRight(Control* self, PyObject* args)
  {
    Control* pControl;
    if (!PyArg_ParseTuple(args, "O", &pControl)) return NULL;
    // type checking, object should be of type Control
    if (!Control_Check(pControl))
    {
      PyErr_SetString(PyExc_TypeError, "Object should be of type Control");
      return NULL;
    }
    if(self->iControlId == 0)
    {
      PyErr_SetString(PyExc_ReferenceError, "Control has to be added to a window first");
      return NULL;
    }

    self->iControlRight = pControl->iControlId;
    PyGUILock();
    if (self->pGUIControl) self->pGUIControl->SetNavigation(self->iControlUp,
      self->iControlDown,self->iControlLeft,self->iControlRight);
    PyGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyMethodDef Control_methods[] = {
    {"getId", (PyCFunction)Control_GetId, METH_VARARGS, getId__doc__},
    {"getPosition", (PyCFunction)Control_GetPosition, METH_VARARGS, getPosition__doc__},
    {"getHeight", (PyCFunction)Control_GetHeight, METH_VARARGS, getHeight__doc__},
    {"getWidth", (PyCFunction)Control_GetWidth, METH_VARARGS, getWidth__doc__},
    {"setEnabled", (PyCFunction)Control_SetEnabled, METH_VARARGS, setEnabled__doc__},
    {"setEnableCondition", (PyCFunction)Control_SetEnableCondition, METH_VARARGS, setEnableCondition__doc__},
    {"setVisible", (PyCFunction)Control_SetVisible, METH_VARARGS, setVisible__doc__},
    {"setVisibleCondition", (PyCFunction)Control_SetVisibleCondition, METH_VARARGS, setVisibleCondition__doc__},
    {"setAnimations", (PyCFunction)Control_SetAnimations, METH_VARARGS, setAnimations__doc__},
    {"setPosition", (PyCFunction)Control_SetPosition, METH_VARARGS, setPosition__doc__},
    {"setWidth", (PyCFunction)Control_SetWidth, METH_VARARGS, setWidth__doc__},
    {"setHeight", (PyCFunction)Control_SetHeight, METH_VARARGS, setHeight__doc__},
    {"setNavigation", (PyCFunction)Control_SetNavigation, METH_VARARGS, setNavigation__doc__},
    {"controlUp", (PyCFunction)Control_ControlUp, METH_VARARGS, controlUp__doc__},
    {"controlDown", (PyCFunction)Control_ControlDown, METH_VARARGS, controlDown__doc__},
    {"controlLeft", (PyCFunction)Control_ControlLeft, METH_VARARGS, controlLeft__doc__},
    {"controlRight", (PyCFunction)Control_ControlRight, METH_VARARGS, controlRight__doc__},
    {NULL, NULL, 0, NULL}
  };

  PyDoc_STRVAR(control__doc__,
    "Control class.\n"
    "\n"
    "Base class for all controls.");

// Restore code and data sections to normal.
#ifndef __GNUC__
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()
#endif

  PyTypeObject Control_Type;

  void initControl_Type()
  {
    PyInitializeTypeObject(&Control_Type);

    Control_Type.tp_name = "xbmcgui.Control";
    Control_Type.tp_basicsize = sizeof(Control);
    Control_Type.tp_dealloc = 0;
    Control_Type.tp_compare = Control_Compare;
    Control_Type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    Control_Type.tp_doc = control__doc__;
    Control_Type.tp_methods = Control_methods;
    Control_Type.tp_base = 0;
    Control_Type.tp_new = 0;
  }
}

#ifdef __cplusplus
}
#endif
