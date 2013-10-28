// Workarounds for iostream symbols that are referenced when building on OS X 10.7 but missing from
// OS X 10.5's stdlibc++.dylib.
//
// In the <iostream> headers these are declared as extern templates but the symbols are not present under 10.5.
// This file forces the compiler to instantiate the templates.
//
// see http://stackoverflow.com/questions/3484043/os-x-program-runs-on-dev-machine-crashing-horribly-on-others

#include <iostream>

_GLIBCXX_BEGIN_NAMESPACE(std)
// From ostream_insert.h
template ostream& __ostream_insert(ostream&, const char*, streamsize);

#ifdef _GLIBCXX_USE_WCHAR_T
template wostream& __ostream_insert(wostream&, const wchar_t*, streamsize);
#endif

// From ostream.tcc
template ostream& ostream::_M_insert(long);
template ostream& ostream::_M_insert(unsigned long);
template ostream& ostream::_M_insert(bool);
#ifdef _GLIBCXX_USE_LONG_LONG
template ostream& ostream::_M_insert(long long);
template ostream& ostream::_M_insert(unsigned long long);
#endif
template ostream& ostream::_M_insert(double);
template ostream& ostream::_M_insert(long double);
template ostream& ostream::_M_insert(const void*);

#ifdef _GLIBCXX_USE_WCHAR_T
template wostream& wostream::_M_insert(long);
template wostream& wostream::_M_insert(unsigned long);
template wostream& wostream::_M_insert(bool);
#ifdef _GLIBCXX_USE_LONG_LONG
template wostream& wostream::_M_insert(long long);
template wostream& wostream::_M_insert(unsigned long long);
#endif
template wostream& wostream::_M_insert(double);
template wostream& wostream::_M_insert(long double);
template wostream& wostream::_M_insert(const void*);
#endif

// From istream.tcc
template istream& istream::_M_extract(unsigned short&);
template istream& istream::_M_extract(unsigned int&);
template istream& istream::_M_extract(long&);
template istream& istream::_M_extract(unsigned long&);
template istream& istream::_M_extract(bool&);
#ifdef _GLIBCXX_USE_LONG_LONG
template istream& istream::_M_extract(long long&);
template istream& istream::_M_extract(unsigned long long&);
#endif
template istream& istream::_M_extract(float&);
template istream& istream::_M_extract(double&);
template istream& istream::_M_extract(long double&);
template istream& istream::_M_extract(void*&);

#ifdef _GLIBCXX_USE_WCHAR_T
template wistream& wistream::_M_extract(unsigned short&);
template wistream& wistream::_M_extract(unsigned int&);
template wistream& wistream::_M_extract(long&);
template wistream& wistream::_M_extract(unsigned long&);
template wistream& wistream::_M_extract(bool&);
#ifdef _GLIBCXX_USE_LONG_LONG
template wistream& wistream::_M_extract(long long&);
template wistream& wistream::_M_extract(unsigned long long&);
#endif
template wistream& wistream::_M_extract(float&);
template wistream& wistream::_M_extract(double&);
template wistream& wistream::_M_extract(long double&);
template wistream& wistream::_M_extract(void*&);
#endif

_GLIBCXX_END_NAMESPACE
