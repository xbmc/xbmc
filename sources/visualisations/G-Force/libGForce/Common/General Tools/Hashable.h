#ifndef _HASHABLE_H
#define _HASHABLE_H


class Hashable {

	public:
		virtual long			Hash() const = 0;
		
		virtual bool			Equals( const Hashable* inComp ) const = 0;
};

#endif