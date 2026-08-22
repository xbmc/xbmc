/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

/* Guard: a container-shaped type that reaches python through SWIG's opaque
   pointer fallback is a build failure, not a runtime TypeError.
   The predicate names the five generic FAMILIES the glue supports (the same
   five it defines traits for), plus a structural catch-all for anything else
   that walks like an STL container. It deliberately does not fire for plain
   wrapped classes, which reach python as pointers by design. */

%header %{
#include "interfaces/legacy/Alternative.h"
#include "interfaces/legacy/Tuple.h"

#include <map>
#include <memory>
#include <type_traits>
#include <vector>

namespace KodiSwig
{
template<class T, class = void>
struct looks_like_container : std::false_type
{
};
template<class T>
struct looks_like_container<T,
                            std::void_t<typename T::value_type,
                                        decltype(std::declval<const T&>().begin()),
                                        decltype(std::declval<const T&>().end())>> : std::true_type
{
};

template<class T>
struct is_generic_family : std::false_type
{
};
template<class... A>
struct is_generic_family<XBMCAddon::Tuple<A...>> : std::true_type
{
};
template<class... A>
struct is_generic_family<XBMCAddon::Alternative<A...>> : std::true_type
{
};

/* smart pointers key on element_type, not value_type, so the container test
   above misses them; a smart pointer to a container must still convert. */
template<class T, class = void>
struct looks_like_smart_ptr : std::false_type
{
};
template<class T>
struct looks_like_smart_ptr<T,
                            std::void_t<typename T::element_type,
                                        decltype(std::declval<const T&>().get())>>
  : std::true_type
{
};

template<class T>
struct needs_conversion
  : std::integral_constant<bool,
                           looks_like_container<T>::value || is_generic_family<T>::value ||
                               looks_like_smart_ptr<T>::value>
{
};
} // namespace KodiSwig

#define KODI_ASSERT_CONVERTED(T) \
  static_assert(!KodiSwig::needs_conversion<T>::value, \
                "this container/generic type reached python as an opaque pointer; " \
                "add a %template() line for it")
%}

/* By-value returns. With anonymous %template() no container method is wrapped,
   so this pattern has no legitimate user and can be poisoned outright. */
%typemap(out, noblock=1) SWIGTYPE
{
  KODI_ASSERT_CONVERTED($1_ltype);
}

/* By-const-reference parameters. This pattern DOES have legitimate users
   (swig::SwigPyIterator, std::less<std::string>, XBMCAddon::xbmcgui::Control),
   so the stock body is preserved and only the assert is added. Body copied from
   the const SWIGTYPE & in-typemap in /usr/share/swig/4.5.0/typemaps/swigtype.swg. */
%typemap(in, noblock=1, implicitconv=1) const SWIGTYPE& (void* argp = 0, int res = 0)
{
  KODI_ASSERT_CONVERTED($*1_ltype);
  res = SWIG_ConvertPtr($input, &argp, $descriptor, %convertptr_flags | %implicitconv_flag);
  if (!SWIG_IsOK(res))
  {
    %argument_fail(res, "$type", $symname, $argnum);
  }
  if (!argp)
  {
    %argument_nullref("$type", $symname, $argnum);
  }
  $1 = %reinterpret_cast(argp, $ltype);
}
