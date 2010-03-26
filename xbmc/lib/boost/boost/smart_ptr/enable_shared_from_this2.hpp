#ifndef BOOST_ENABLE_SHARED_FROM_THIS2_HPP_INCLUDED
#define BOOST_ENABLE_SHARED_FROM_THIS2_HPP_INCLUDED

//
//  enable_shared_from_this2.hpp
//
//  Copyright 2002, 2009 Peter Dimov
//  Copyright 2008 Frank Mori Hess
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//

#include <boost/config.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/assert.hpp>
#include <boost/detail/workaround.hpp>

namespace boost
{

namespace detail
{

class esft2_deleter_wrapper
{
private:

    shared_ptr<void> deleter_;

public:

    esft2_deleter_wrapper()
    {
    }

    template< class T > void set_deleter( shared_ptr<T> const & deleter )
    {
        deleter_ = deleter;
    }

    template< class T> void operator()( T* )
    {
        BOOST_ASSERT( deleter_.use_count() <= 1 );
        deleter_.reset();
    }
};

} // namespace detail

template< class T > class enable_shared_from_this2
{
protected:

    enable_shared_from_this2()
    {
    }

    enable_shared_from_this2( enable_shared_from_this2 const & )
    {
    }

    enable_shared_from_this2 & operator=( enable_shared_from_this2 const & )
    {
        return *this;
    }

    ~enable_shared_from_this2()
    {
        BOOST_ASSERT( shared_this_.use_count() <= 1 ); // make sure no dangling shared_ptr objects exist
    }

private:

    mutable weak_ptr<T> weak_this_;
    mutable shared_ptr<T> shared_this_;

public:

    shared_ptr<T> shared_from_this()
    {
        init_weak_once();
        return shared_ptr<T>( weak_this_ );
    }

    shared_ptr<T const> shared_from_this() const
    {
        init_weak_once();
        return shared_ptr<T>( weak_this_ );
    }

private:

    void init_weak_once() const
    {
        if( weak_this_._empty() )
        {
            shared_this_.reset( static_cast< T* >( 0 ), detail::esft2_deleter_wrapper() );
            weak_this_ = shared_this_;
        }
    }

public: // actually private, but avoids compiler template friendship issues

    // Note: invoked automatically by shared_ptr; do not call
    template<class X, class Y> void _internal_accept_owner( shared_ptr<X> * ppx, Y * py ) const
    {
        BOOST_ASSERT( ppx != 0 );

        if( weak_this_.use_count() == 0 )
        {
            weak_this_ = shared_ptr<T>( *ppx, py );
        }
        else if( shared_this_.use_count() != 0 )
        {
            BOOST_ASSERT( ppx->unique() ); // no weak_ptrs should exist either, but there's no way to check that

            detail::esft2_deleter_wrapper * pd = boost::get_deleter<detail::esft2_deleter_wrapper>( shared_this_ );
            BOOST_ASSERT( pd != 0 );

            pd->set_deleter( *ppx );

            ppx->reset( shared_this_, ppx->get() );
            shared_this_.reset();
        }
    }
};

} // namespace boost

#endif  // #ifndef BOOST_ENABLE_SHARED_FROM_THIS2_HPP_INCLUDED
