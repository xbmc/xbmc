// Copyright (C) 2007 Fokko Beekhof
// Email contact: Fokko.Beekhof@cui.unige.ch

// The OMPTL library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.

// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

namespace omptl
{

// transform_accumulate
template <class Iterator,class T, class UnaryFunction,class BinaryFunction>
T transform_accumulate(Iterator first, Iterator last, T init,
		UnaryFunction unary_op, BinaryFunction binary_op,
		const unsigned P)
{
	// serial version
	while (first != last)
	{
		init = binary_op(unary_op(*first), init);
		++first;
	}

	return init;
}

template <class Iterator, class T, class UnaryFunction>
T transform_accumulate(Iterator first, Iterator last,
		T init, UnaryFunction unary_op,
		const unsigned P)
{
	return omptl::transform_accumulate(first, last, init, unary_op,
					   std::plus<T>());
}

} /* namespace std */
