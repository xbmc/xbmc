/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

/* Kodi's own generic vocabulary types, taught to SWIG the same way the SWIG
   library teaches it std::vector: the conversion code is C++ partial
   specializations of swig::traits_asptr / swig::traits_from that recurse
   through swig::as<T> / swig::from<T>, and the typemaps are declared once on
   the template pattern.  Nothing is keyed on a type spelling. */

%include <std_common.i>
%include <std_map.i>
%include <std_vector.i>

/* ------------------------------------------------------------------ */
/* Tuple                                                              */
/* ------------------------------------------------------------------ */

%fragment("KodiTupleTraits", "header", fragment="StdTraits") {
namespace swig {

  template <class T1>
  struct traits< XBMCAddon::Tuple<T1> > {
    typedef pointer_category category;
    static const char* type_name() { return "XBMCAddon::Tuple"; }
  };
  template <class T1, class T2>
  struct traits< XBMCAddon::Tuple<T1,T2> > {
    typedef pointer_category category;
    static const char* type_name() { return "XBMCAddon::Tuple"; }
  };
  template <class T1, class T2, class T3>
  struct traits< XBMCAddon::Tuple<T1,T2,T3> > {
    typedef pointer_category category;
    static const char* type_name() { return "XBMCAddon::Tuple"; }
  };
  template <class T1, class T2, class T3, class T4>
  struct traits< XBMCAddon::Tuple<T1,T2,T3,T4> > {
    typedef pointer_category category;
    static const char* type_name() { return "XBMCAddon::Tuple"; }
  };

  /* out: a python tuple of exactly GetNumValuesSet() entries */
  template <class T1>
  struct traits_from< XBMCAddon::Tuple<T1> > {
    static PyObject *from(const XBMCAddon::Tuple<T1>& t) {
      int n = t.GetNumValuesSet();
      PyObject *o = PyTuple_New(n);
      if (n > 0) PyTuple_SetItem(o, 0, swig::from<T1>(t.first()));
      return o;
    }
  };
  template <class T1, class T2>
  struct traits_from< XBMCAddon::Tuple<T1,T2> > {
    static PyObject *from(const XBMCAddon::Tuple<T1,T2>& t) {
      int n = t.GetNumValuesSet();
      PyObject *o = PyTuple_New(n);
      if (n > 0) PyTuple_SetItem(o, 0, swig::from<T1>(t.first()));
      if (n > 1) PyTuple_SetItem(o, 1, swig::from<T2>(t.second()));
      return o;
    }
  };
  template <class T1, class T2, class T3>
  struct traits_from< XBMCAddon::Tuple<T1,T2,T3> > {
    static PyObject *from(const XBMCAddon::Tuple<T1,T2,T3>& t) {
      int n = t.GetNumValuesSet();
      PyObject *o = PyTuple_New(n);
      if (n > 0) PyTuple_SetItem(o, 0, swig::from<T1>(t.first()));
      if (n > 1) PyTuple_SetItem(o, 1, swig::from<T2>(t.second()));
      if (n > 2) PyTuple_SetItem(o, 2, swig::from<T3>(t.third()));
      return o;
    }
  };

  /* in: any python sequence; a short sequence leaves later slots unset */
  template <class T1>
  struct traits_asptr< XBMCAddon::Tuple<T1> > {
    static int asptr(PyObject *obj, XBMCAddon::Tuple<T1> **val) {
      if (!obj || !PySequence_Check(obj)) return SWIG_ERROR;
      XBMCAddon::Tuple<T1> *t = new XBMCAddon::Tuple<T1>();
      Py_ssize_t n = PySequence_Size(obj);
      try {
        if (n > 0) { SwigVar_PyObject e = PySequence_GetItem(obj,0); t->first() = swig::as<T1>(e); }
      } catch (std::exception&) { delete t; PyErr_Clear(); return SWIG_ERROR; }
      if (val) *val = t; else delete t;
      return val ? SWIG_NEWOBJ : SWIG_OK;
    }
  };
  template <class T1, class T2>
  struct traits_asptr< XBMCAddon::Tuple<T1,T2> > {
    static int asptr(PyObject *obj, XBMCAddon::Tuple<T1,T2> **val) {
      if (!obj || !PySequence_Check(obj)) return SWIG_ERROR;
      XBMCAddon::Tuple<T1,T2> *t = new XBMCAddon::Tuple<T1,T2>();
      Py_ssize_t n = PySequence_Size(obj);
      try {
        if (n > 0) { SwigVar_PyObject e = PySequence_GetItem(obj,0); t->first()  = swig::as<T1>(e); }
        if (n > 1) { SwigVar_PyObject e = PySequence_GetItem(obj,1); t->second() = swig::as<T2>(e); }
      } catch (std::exception&) { delete t; PyErr_Clear(); return SWIG_ERROR; }
      if (val) *val = t; else delete t;
      return val ? SWIG_NEWOBJ : SWIG_OK;
    }
  };
  template <class T1, class T2, class T3>
  struct traits_asptr< XBMCAddon::Tuple<T1,T2,T3> > {
    static int asptr(PyObject *obj, XBMCAddon::Tuple<T1,T2,T3> **val) {
      if (!obj || !PySequence_Check(obj)) return SWIG_ERROR;
      XBMCAddon::Tuple<T1,T2,T3> *t = new XBMCAddon::Tuple<T1,T2,T3>();
      Py_ssize_t n = PySequence_Size(obj);
      try {
        if (n > 0) { SwigVar_PyObject e = PySequence_GetItem(obj,0); t->first()  = swig::as<T1>(e); }
        if (n > 1) { SwigVar_PyObject e = PySequence_GetItem(obj,1); t->second() = swig::as<T2>(e); }
        if (n > 2) { SwigVar_PyObject e = PySequence_GetItem(obj,2); t->third()  = swig::as<T3>(e); }
      } catch (std::exception&) { delete t; PyErr_Clear(); return SWIG_ERROR; }
      /* An unset element stays value-initialised, so a pointer slot is null.
         Callees do not all check GetNumValuesSet: addDirectoryItems guards its
         third element but dereferences the second unconditionally. Refuse the
         short sequence here rather than hand C++ a null it will dereference. */
      if (n < 2) { delete t; return SWIG_ERROR; }
      if (val) *val = t; else delete t;
      return val ? SWIG_NEWOBJ : SWIG_OK;
    }
  };
}
}

/* ------------------------------------------------------------------ */
/* Alternative                                                        */
/* ------------------------------------------------------------------ */

%fragment("KodiAlternativeTraits", "header", fragment="StdTraits") {
namespace swig {
  template <class T1, class T2>
  struct traits< XBMCAddon::Alternative<T1,T2> > {
    typedef pointer_category category;
    static const char* type_name() { return "XBMCAddon::Alternative"; }
  };

  template <class T1, class T2>
  struct traits_from< XBMCAddon::Alternative<T1,T2> > {
    static PyObject *from(const XBMCAddon::Alternative<T1,T2>& a) {
      if (a.which() == XBMCAddon::first)  return swig::from<T1>(a.former());
      if (a.which() == XBMCAddon::second) return swig::from<T2>(a.later());
      Py_INCREF(Py_None);
      return Py_None;
    }
  };

  template <class T1, class T2>
  struct traits_asptr< XBMCAddon::Alternative<T1,T2> > {
    static int asptr(PyObject *obj, XBMCAddon::Alternative<T1,T2> **val) {
      if (!obj) return SWIG_ERROR;
      XBMCAddon::Alternative<T1,T2> *a = new XBMCAddon::Alternative<T1,T2>();
      try {
        a->former() = swig::as<T1>(obj);
      } catch (std::exception&) {
        PyErr_Clear();
        try {
          XBMCAddon::Alternative<T1,T2> *b = new XBMCAddon::Alternative<T1,T2>();
          b->later() = swig::as<T2>(obj);
          delete a; a = b;
        } catch (std::exception&) {
          delete a; PyErr_Clear(); return SWIG_ERROR;
        }
      }
      if (val) *val = a; else delete a;
      return val ? SWIG_NEWOBJ : SWIG_OK;
    }
  };
}
}

/* ------------------------------------------------------------------ */
/* Dictionary: a std::map with a transparent comparator               */
/* ------------------------------------------------------------------ */

%fragment("KodiDictTraits", "header", fragment="StdTraits", fragment="SWIG_AsVal_std_string") {
namespace swig {
  template <class T>
  struct traits< XBMCAddon::Dictionary<T> > {
    typedef pointer_category category;
    static const char* type_name() { return "XBMCAddon::Dictionary"; }
  };

  /* The key is always a string, so only the value type recurses. */
  template <class T>
  struct traits_from< XBMCAddon::Dictionary<T> > {
    static PyObject *from(const XBMCAddon::Dictionary<T>& d) {
      PyObject *o = PyDict_New();
      for (typename XBMCAddon::Dictionary<T>::const_iterator it = d.begin(); it != d.end(); ++it) {
        SwigVar_PyObject k = PyUnicode_DecodeUTF8(it->first.c_str(),
                                                  (Py_ssize_t)it->first.size(), "surrogateescape");
        SwigVar_PyObject v = swig::from<T>(it->second);
        if (!k || !v) { Py_DECREF(o); return NULL; }
        PyDict_SetItem(o, k, v);
      }
      return o;
    }
  };

  template <class T>
  struct traits_asptr< XBMCAddon::Dictionary<T> > {
    typedef XBMCAddon::Dictionary<T> dict;
    static int asptr(PyObject *obj, dict **val) {
      if (!obj || !PyDict_Check(obj)) return SWIG_ERROR;
      if (!val) return SWIG_OK;
      dict *d = new dict();
      PyObject *k, *v;
      Py_ssize_t pos = 0;
      while (PyDict_Next(obj, &pos, &k, &v)) {
        std::string key;
        int kr = SWIG_AsVal_std_string(k, &key);
        if (!SWIG_IsOK(kr)) { delete d; return SWIG_ERROR; }
        try {
          /* Values are StringOrInt: the Groovy generator keyed on that typedef
             name to coerce numbers with str(). The typedef is a no-op in C++,
             so stock SWIG cannot see it and an int value would be rejected.
             Scrapers pass ints routinely, e.g. setInfo("video", {"year": 2020}). */
          if (PyLong_Check(v) || PyFloat_Check(v)) {
            SwigVar_PyObject sv = PyObject_Str(v);
            if (!sv) { delete d; PyErr_Clear(); return SWIG_ERROR; }
            (*d)[key] = swig::as<T>(sv);
          } else {
            (*d)[key] = swig::as<T>(v);
          }
        } catch (std::exception&) { delete d; PyErr_Clear(); return SWIG_ERROR; }
      }
      *val = d;
      return SWIG_NEWOBJ;
    }
  };
}
}

/* ------------------------------------------------------------------ */
/* SWIG-visible declarations that carry the typemap patterns          */
/* ------------------------------------------------------------------ */

namespace XBMCAddon {

  struct tuple_null_type {};

  template<class T1 = XBMCAddon::tuple_null_type,
           class T2 = XBMCAddon::tuple_null_type,
           class T3 = XBMCAddon::tuple_null_type,
           class T4 = XBMCAddon::tuple_null_type,
           class E  = XBMCAddon::tuple_null_type>
  class Tuple {
  public:
    %traits_swigtype(T1);
    %traits_swigtype(T2);
    %traits_swigtype(T3);
    %traits_swigtype(T4);

    %fragment(SWIG_Traits_frag(XBMCAddon::Tuple< T1,T2,T3,T4,E >), "header",
              fragment=SWIG_Traits_frag(T1),
              fragment=SWIG_Traits_frag(T2),
              fragment=SWIG_Traits_frag(T3),
              fragment=SWIG_Traits_frag(T4),
              fragment="KodiTupleTraits") { }

    %typemap_traits_ptr(SWIG_TYPECHECK_POINTER, XBMCAddon::Tuple< T1,T2,T3,T4,E >);
  };

  template<class T1, class T2>
  class Alternative {
  public:
    %traits_swigtype(T1);
    %traits_swigtype(T2);

    %fragment(SWIG_Traits_frag(XBMCAddon::Alternative< T1,T2 >), "header",
              fragment=SWIG_Traits_frag(T1),
              fragment=SWIG_Traits_frag(T2),
              fragment="KodiAlternativeTraits") { }

    %typemap_traits_ptr(SWIG_TYPECHECK_POINTER, XBMCAddon::Alternative< T1,T2 >);
  };

  template<class T>
  class Dictionary {
  public:
    %traits_swigtype(T);

    %fragment(SWIG_Traits_frag(XBMCAddon::Dictionary< T >), "header",
              fragment=SWIG_Traits_frag(T),
              fragment="KodiDictTraits") { }

    %typemap_traits_ptr(SWIG_TYPECHECK_MAP, XBMCAddon::Dictionary< T >);
  };
}


/* ControlList::setStaticContent takes a raw pointer to a container:
       void setStaticContent(const ListItemList* items);
   %ptr_in_typemap generates patterns for Type and const Type& only, so there is
   no const Type* pattern for a %template() line to reach; the parameter falls
   to the opaque pointer typemap and python cannot call the method at all.
   Convert into a local and pass its address. */
%typemap(in) const XBMCAddon::xbmcgui::ListItemList *
             (XBMCAddon::xbmcgui::ListItemList swig_temp) {
  XBMCAddon::xbmcgui::ListItemList *swig_p = 0;
  int swig_res = swig::asptr($input, &swig_p);
  if (!SWIG_IsOK(swig_res) || !swig_p)
    SWIG_exception_fail(SWIG_ArgError(SWIG_TypeError),
      "in method '$symname', argument $argnum of type '$type'");
  swig_temp = *swig_p;
  if (SWIG_IsNewObj(swig_res)) delete swig_p;
  $1 = &swig_temp;
}
%typemap(freearg) const XBMCAddon::xbmcgui::ListItemList * ""
