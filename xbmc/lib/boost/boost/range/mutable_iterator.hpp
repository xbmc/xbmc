// Boost.Range library
//
//  Copyright Thorsten Ottosen 2003-2004. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/range/
//

#ifndef BOOST_RANGE_MUTABLE_ITERATOR_HPP
#define BOOST_RANGE_MUTABLE_ITERATOR_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include <boost/range/config.hpp>

#ifdef BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION
#include <boost/range/detail/iterator.hpp>
#else

#include <boost/iterator/iterator_traits.hpp>
#include <cstddef>
#include <utility>

namespace boost
{
    //////////////////////////////////////////////////////////////////////////
    // default
    //////////////////////////////////////////////////////////////////////////
    
    template< typename C >
    struct range_mutable_iterator
    {
        typedef BOOST_DEDUCED_TYPENAME C::iterator type;
    };
    
    //////////////////////////////////////////////////////////////////////////
    // pair
    //////////////////////////////////////////////////////////////////////////

    template< typename Iterator >
    struct range_mutable_iterator< std::pair<Iterator,Iterator> >
    {
        typedef Iterator type;
    };

    //////////////////////////////////////////////////////////////////////////
    // array
    //////////////////////////////////////////////////////////////////////////

    template< typename T, std::size_t sz >
    struct range_mutable_iterator< T[sz] >
    {
        typedef T* type;
    };

} // namespace boost

#endif // BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION

#endif
