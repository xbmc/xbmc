#ifndef BOOST_MEMORY_ORDER_HPP_INCLUDED
#define BOOST_MEMORY_ORDER_HPP_INCLUDED

// MS compatible compilers support #pragma once

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

//  boost/memory_order.hpp
//
//  Defines enum boost::memory_order per the C++0x working draft
//
//  Copyright (c) 2008 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)


namespace boost
{

enum memory_order
{
    memory_order_relaxed = 0,
    memory_order_acquire = 1,
    memory_order_release = 2,
    memory_order_acq_rel = 3, // acquire | release
    memory_order_seq_cst = 7  // acq_rel | 4
};

} // namespace boost

#endif // #ifndef BOOST_MEMORY_ORDER_HPP_INCLUDED
