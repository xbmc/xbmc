// (C) Copyright 2002-2008, Fernando Luis Cacciola Carballal.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// 21 Ago 2002 (Created) Fernando Cacciola
// 24 Dec 2007 (Refactored and worked around various compiler bugs) Fernando Cacciola, Niels Dekker
// 23 May 2008 (Fixed operator= const issue, added initialized_value) Niels Dekker, Fernando Cacciola
// 21 Ago 2008 (Added swap) Niels Dekker, Fernando Cacciola
// 20 Feb 2009 (Fixed logical const-ness issues) Niels Dekker, Fernando Cacciola
//
#ifndef BOOST_UTILITY_VALUE_INIT_21AGO2002_HPP
#define BOOST_UTILITY_VALUE_INIT_21AGO2002_HPP

// Note: The implementation of boost::value_initialized had to deal with the
// fact that various compilers haven't fully implemented value-initialization.
// The constructor of boost::value_initialized<T> works around these compiler
// issues, by clearing the bytes of T, before constructing the T object it
// contains. More details on these issues are at libs/utility/value_init.htm

#include <boost/aligned_storage.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits/cv_traits.hpp>
#include <boost/type_traits/alignment_of.hpp>
#include <boost/swap.hpp>
#include <cstring>
#include <new>

namespace boost {

template<class T>
class value_initialized
{
  private :
    struct wrapper
    {
#if !BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x592))
      typename
#endif 
      remove_const<T>::type data;
    };

    mutable
#if !BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x592))
      typename
#endif 
      aligned_storage<sizeof(wrapper), alignment_of<wrapper>::value>::type x;

    wrapper * wrapper_address() const
    {
      return static_cast<wrapper *>( static_cast<void*>(&x));
    }

  public :

    value_initialized()
    {
      std::memset(&x, 0, sizeof(x));
#ifdef BOOST_MSVC
#pragma warning(push)
#if _MSC_VER >= 1310
// When using MSVC 7.1 or higher, the following placement new expression may trigger warning C4345:
// "behavior change: an object of POD type constructed with an initializer of the form ()
// will be default-initialized".  It is safe to ignore this warning when using value_initialized.
#pragma warning(disable: 4345)
#endif
#endif
      new (wrapper_address()) wrapper();
#ifdef BOOST_MSVC
#pragma warning(pop)
#endif
    }

    value_initialized(value_initialized const & arg)
    {
      new (wrapper_address()) wrapper( static_cast<wrapper const &>(*(arg.wrapper_address())));
    }

    value_initialized & operator=(value_initialized const & arg)
    {
      // Assignment is only allowed when T is non-const.
      BOOST_STATIC_ASSERT( ! is_const<T>::value );
      *wrapper_address() = static_cast<wrapper const &>(*(arg.wrapper_address()));
      return *this;
    }

    ~value_initialized()
    {
      wrapper_address()->wrapper::~wrapper();
    }

    T const & data() const
    {
      return wrapper_address()->data;
    }

    T& data()
    {
      return wrapper_address()->data;
    }

    void swap(value_initialized & arg)
    {
      ::boost::swap( this->data(), arg.data() );
    }

    operator T const &() const { return this->data(); }

    operator T&() { return this->data(); }

} ;





template<class T>
T const& get ( value_initialized<T> const& x )
{
  return x.data() ;
}
template<class T>
T& get ( value_initialized<T>& x )
{
  return x.data() ;
}

template<class T>
void swap ( value_initialized<T> & lhs, value_initialized<T> & rhs )
{
  lhs.swap(rhs) ;
}


class initialized_value_t
{
  public :
    
    template <class T> operator T() const
    {
      return get( value_initialized<T>() );
    }
};

initialized_value_t const initialized_value = {} ;


} // namespace boost


#endif
