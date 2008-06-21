#ifndef BOOST_DETAIL_SP_TYPEINFO_HPP_INCLUDED
#define BOOST_DETAIL_SP_TYPEINFO_HPP_INCLUDED

// MS compatible compilers support #pragma once

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

//  detail/sp_typeinfo.hpp
//
//  Copyright 2007 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config.hpp>

#if defined( BOOST_NO_TYPEID )

namespace boost
{

namespace detail
{

typedef void* sp_typeinfo;

template<class T> struct sp_typeid_
{
    static char v_;
};

template<class T> char sp_typeid_< T >::v_;

template<class T> struct sp_typeid_< T const >: sp_typeid_< T >
{
};

template<class T> struct sp_typeid_< T volatile >: sp_typeid_< T >
{
};

template<class T> struct sp_typeid_< T const volatile >: sp_typeid_< T >
{
};

} // namespace detail

} // namespace boost

#define BOOST_SP_TYPEID(T) (&boost::detail::sp_typeid_<T>::v_)

#else

#include <typeinfo>

namespace boost
{

namespace detail
{

#if defined( BOOST_NO_STD_TYPEINFO )

typedef ::type_info sp_typeinfo;

#else

typedef std::type_info sp_typeinfo;

#endif

} // namespace detail

} // namespace boost

#define BOOST_SP_TYPEID(T) typeid(T)

#endif

#endif  // #ifndef BOOST_DETAIL_SP_TYPEINFO_HPP_INCLUDED
