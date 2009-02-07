#ifndef BOOST_THROW_EXCEPTION_HPP_INCLUDED
#define BOOST_THROW_EXCEPTION_HPP_INCLUDED

// MS compatible compilers support #pragma once

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

//
//  boost/throw_exception.hpp
//
//  Copyright (c) 2002 Peter Dimov and Multi Media Ltd.
//  Copyright (c) 2008 Emil Dotchevski and Reverge Studios, Inc.
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//  http://www.boost.org/libs/utility/throw_exception.html
//

#include <boost/config.hpp>
#include <boost/detail/workaround.hpp>
#include <exception>

#if !defined( BOOST_EXCEPTION_DISABLE ) && defined( __BORLANDC__ ) && BOOST_WORKAROUND( __BORLANDC__, BOOST_TESTED_AT(0x593) )
# define BOOST_EXCEPTION_DISABLE
#endif

#if !defined( BOOST_EXCEPTION_DISABLE ) && defined( BOOST_MSVC ) && BOOST_WORKAROUND( BOOST_MSVC, < 1310 )
# define BOOST_EXCEPTION_DISABLE
#endif

#if !defined( BOOST_NO_EXCEPTIONS ) && !defined( BOOST_EXCEPTION_DISABLE )
# include <boost/exception/exception.hpp>
# include <boost/current_function.hpp>
# define BOOST_THROW_EXCEPTION(x) ::boost::throw_exception(::boost::enable_error_info(x) <<\
    ::boost::throw_function(BOOST_CURRENT_FUNCTION) <<\
    ::boost::throw_file(__FILE__) <<\
    ::boost::throw_line((int)__LINE__))
#else
# define BOOST_THROW_EXCEPTION(x) ::boost::throw_exception(x)
#endif

namespace boost
{

#ifdef BOOST_NO_EXCEPTIONS

void throw_exception( std::exception const & e ); // user defined

#else

inline void throw_exception_assert_compatibility( std::exception const & ) { }

template<class E> inline void throw_exception( E const & e )
{
    //All boost exceptions are required to derive std::exception,
    //to ensure compatibility with BOOST_NO_EXCEPTIONS.
    throw_exception_assert_compatibility(e);

#ifndef BOOST_EXCEPTION_DISABLE
    throw enable_current_exception(enable_error_info(e));
#else
    throw e;
#endif
}

#endif

} // namespace boost

#endif // #ifndef BOOST_THROW_EXCEPTION_HPP_INCLUDED
