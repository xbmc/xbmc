/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

/* Sequences return as python lists, not tuples. Fragments are first-definition-wins,
   so this must precede every stock include. */
%fragment("StdVectorTraits","header",fragment="StdSequenceTraits")
%{
  #include <type_traits>

  namespace swig {
    // python owns each AddonClass element, as %newobject does for a single return
    template <class T>
    PyObject *kodi_from_element(const T& val) {
      using Pointee = std::remove_cv_t<std::remove_pointer_t<T>>;
      if constexpr (std::is_pointer_v<T> && std::is_base_of_v<XBMCAddon::AddonClass, Pointee>) {
        Pointee *obj = const_cast<Pointee *>(val);
        PyObject *result = traits_from_ptr<Pointee>::from(obj, SWIG_POINTER_OWN);
        if (result)
          KodiSwig_acquire(obj);
        return result;
      } else {
        return swig::from<T>(val);
      }
    }

    template <class T>
    struct traits_reserve<std::vector<T> > {
      static void reserve(std::vector<T> &seq, typename std::vector<T>::size_type n) {
        seq.reserve(n);
      }
    };
    template <class T>
    struct traits_asptr<std::vector<T> >  {
      static int asptr(PyObject *obj, std::vector<T> **vec) {
        return traits_asptr_stdseq<std::vector<T> >::asptr(obj, vec);
      }
    };
    template <class T>
    struct traits_from<std::vector<T> > {
      static PyObject *from(const std::vector<T>& vec) {
        PyObject *lst = PyList_New((Py_ssize_t)vec.size());
        if (!lst) return NULL;
        Py_ssize_t i = 0;
        for (typename std::vector<T>::const_iterator it = vec.begin(); it != vec.end(); ++it, ++i)
          PyList_SetItem(lst, i, kodi_from_element<T>(*it));
        return lst;
      }
    };
  }
%}
