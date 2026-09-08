/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

/* Kodi returns python lists, not tuples, for sequence-valued results.
   SWIG fragments are first-definition-wins, so defining StdVectorTraits here,
   ahead of every stock include, replaces the library's PyTuple_New version. */
%fragment("StdVectorTraits","header",fragment="StdSequenceTraits")
%{
  namespace swig {
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
          PyList_SetItem(lst, i, swig::from<T>(*it));
        return lst;
      }
    };
  }
%}
