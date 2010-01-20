/***************************************************************************************************
* Program:    Miscellaneous Classes                                                                *
* Programmer: Anti Resonance                                                                       *
*                                                                                                  *
* This library is free software; you can redistribute it and/or modify it under the terms of the   *
* GNU Lesser General Public License as published by the Free Software Foundation; either version   *
* 2.1 of the License, or (at your option) any later version.                                       *
*                                                                                                  *
* This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;        *
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.        *
* See the GNU Lesser General Public License for more details.                                      *
*                                                                                                  *
* You should have received a copy of the GNU Lesser General Public License along with this         *
* library; if not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330,        *
* Boston, MA  02111-1307  USA                                                                      *
*                                                                                                  *
*                                                           Copyright (C)2006 Alpha-II Productions *
***************************************************************************************************/

#include "Types.h"

namespace A2
{

	////////////////////////////////////////////////////////////////////////////////////////////////
	// Set Class Template
	//
	// Used to maintain a set of integral values from 0 to 31.  For a larger range or for sets
	// containing negative values, use SetL.  Behaviour is undefined for Sets of a non-integral
	// type.  No range checking is performed.

	template<typename T>
	class Set
	{
		u32		bits;

	public:
		inline			Set<T>()						{bits = 0;}
		inline			Set<T>(const Set<T>& s)			{bits = s.bits;}
		inline			Set<T>(const T i)				{bits = 1 << i;}
						Set<T>(u32 num, const T* items)
	  					{	bits = 0; while (num) bits |= 1 << items[--num]; }

		inline void		Clear()							{bits = 0;}

		inline bool		HasItems() const				{return bits != 0;}
		u32				NumItems() const
						{	u32 i=0,j=bits; while (j) {i += (j & 1); j >>= 1;} return i; }

		inline Set<T>&	operator =  (const Set<T> s)	{bits = s; return *this;}
		inline Set<T>&	operator << (const Set<T> s)	{bits |= s.bits; return *this;}
		inline Set<T>&	operator >> (const Set<T> s)	{bits &= ~s.bits; return *this;}
		inline Set<T>	operator ~  () const			{Set<T> t; t.bits = ~bits; return t;}

		inline Set<T>&	operator =  (const T i)			{bits = 1 << i; return *this;}
		inline Set<T>&	operator << (const T i)			{bits |= 1 << i; return *this;}
		inline Set<T>&	operator >> (const T i)			{bits &= ~(1 << i); return *this;}
		inline bool		operator [] (const T i) const	{return (bits & (1 << i)) != 0;}

		inline bool		operator && (const Set<T> s) const	{return (bits & s.bits) == s.bits;}
		inline bool		operator || (const Set<T> s) const	{return (bits & s.bits) != 0;}
	};



	////////////////////////////////////////////////////////////////////////////////////////////////
	// Large Set Class Template
	//
	// Like a Set, but can contain much larger ranges (-2^31 to 2^31-1).  Range checking _is_
	// performed on the input.

#ifndef _MSC_VER								//MS VC++ 6 doesn't like this class
	template<typename T, T start, T end>
	class SetL
	{
	private:
		u8		bits[((end - start) + 8) >> 3];

	public:
		inline			SetL<T,start,end>()				{Clear();}

		inline			SetL<T,start,end>(const SetL<T,start,end>& s)
						{	operator = (s); }

						SetL<T,start,end>(u32 num, const s32* items)
						{	Clear(); while (num) operator << (items[--num]); }


		void			Clear()
						{	for (u32 i=((end - start) + 7) >> 3; i;) bits[--i] = 0; }


		inline SetL<T,start,end>&	operator << (const s32 i)
						{	if (i >= start && i <= end)
							{	const s32	b = i - start; bits[b >> 3] |= 1 << (b & 7); }
							return *this; }

		inline SetL<T,start,end>&	operator >> (const s32 i)
						{	if (i >= start && i <= end)
							{	const s32	b = i - start; bits[b >> 3] &= ~(1 << (b & 7)); }
							return *this; }

		inline bool		operator [] (const s32 i) const
						{	if (i >= start && i <= end)
							{	const s32	b = i - start;
								return (bits[b >> 3] & (1 << (b & 7))) != 0; }
							return false; }
	};
#endif


	////////////////////////////////////////////////////////////////////////////////////////////////
	// Color Class

	class Color
	{
		union
		{
			struct {u8 r,g,b,a;};
			u32	rgba;
		};

	public:
		inline Color&	operator = (const Color& c)
						{this->rgba = c.rgba; return *this;}

		inline Color&	operator = (const u32 rgba)
						{this->rgba = rgba; return *this;}

		inline void		operator () (const u8 r, const u8 g, const u8 b, const u8 a = 255)
						{this->r = r; this->g = g; this->b = b; this->a = a;}

//		inline void		operator () (const f32 r, const f32 g, const f32 b, const f32 a = 1.0f)
//						{	this->r = (u8)(r * 255.0f); this->g = (u8)(g * 255.0f);
//							this->b = (u8)(b * 255.0f); this->a = (u8)(a * 255.0f); }

		inline			Color()					{}
		inline			Color(const Color& c)	{operator = (c.rgba);}
		inline			Color(const u32 rgba)	{operator = (rgba);}
		inline			Color(const u8 r, const u8 g, const u8 b, const u8 a = 255)
												{operator () (r, g, b, a);}
//		inline			Color(const f32 r, const f32 g, const f32 b, const f32 a = 1.0f)
//												{operator () (r, g, b, a);}

		inline void		Red(const u8 val)		{r = val;}
		inline void		Green(const u8 val)		{g = val;}
		inline void		Blue(const u8 val)		{b = val;}
		inline void		Alpha(const u8 val)		{a = val;}

		inline u8		Red() const				{return r;}
		inline u8		Green() const			{return g;}
		inline u8		Blue() const			{return b;}
		inline u8		Alpha() const			{return a;}

		inline			operator u32 () const	{return rgba;}

#if defined __BORLANDC__
		inline			operator TColor () const
						{return static_cast<TColor>(rgba & 0xFFFFFF);}
#endif

		inline Color	operator >> (const u8 sa) const
						{return Color((u8)(r >> sa), (u8)(g >> sa), (u8)(b >> sa), (u8)(a >> sa));}

		inline void		Avg(const Color c1, const Color c2)
						{	r = (u8)(((u32)c1.r + (u32)c2.r) >> 1);
							g = (u8)(((u32)c1.g + (u32)c2.g) >> 1);
							b = (u8)(((u32)c1.b + (u32)c2.b) >> 1);
							a = (u8)(((u32)c1.a + (u32)c2.a) >> 1);}

	};

}	//namespace A2
