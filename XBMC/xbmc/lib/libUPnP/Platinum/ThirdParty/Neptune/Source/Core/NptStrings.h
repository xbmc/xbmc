/*****************************************************************
|
|   Neptune - String Objects
|
|   (c) 2001-2003 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _NPT_STRINGS_H_
#define _NPT_STRINGS_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptConfig.h"
#if defined(NPT_CONFIG_HAVE_NEW_H)
#include <new>
#endif
#include "NptTypes.h"
#include "NptConstants.h"
#include "NptDebug.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const int NPT_STRING_SEARCH_FAILED = -1;

/*----------------------------------------------------------------------
|   NPT_String
+---------------------------------------------------------------------*/
class NPT_String
{
public:
    // factories
    static NPT_String FromInteger(long value);
    static NPT_String FromIntegerU(unsigned long value);

    // constructors
    NPT_String(const NPT_String& str);
    NPT_String(const char* str);
    NPT_String(const char* str, NPT_Size length);
    NPT_String(const char* str, NPT_Ordinal first, NPT_Size length);
    NPT_String(char c, NPT_Cardinal repeat = 1);
    NPT_String() : m_Chars(NULL) {}
   ~NPT_String() { if (m_Chars) delete GetBuffer(); }

    // string info and manipulations
    bool       IsEmpty() const { return m_Chars == NULL || GetBuffer()->GetLength() == 0; }
    NPT_Size   GetLength()   const { return m_Chars ? GetBuffer()->GetLength() : 0;    }
    NPT_Size   GetCapacity() const { return m_Chars ? GetBuffer()->GetAllocated() : 0; }
    NPT_Result SetLength(NPT_Size length);
    void       Assign(const char* chars, NPT_Size size);
    void       Append(const char* chars, NPT_Size size);
    void       Append(const char* s) { Append(s, StringLength(s)); }
    int        Compare(const char* s, bool ignore_case = false) const;
    static int Compare(const char* s1, const char* s2, bool ignore_case = false);
    int        CompareN(const char* s, NPT_Size count, bool ignore_case = false) const;
    static int CompareN(const char* s1, const char* s2, NPT_Size count, bool ignore_case = false);

    // substrings
    NPT_String SubString(NPT_Ordinal first, NPT_Size length) const;
    NPT_String SubString(NPT_Ordinal first) const {
        return SubString(first, GetLength());
    }
    NPT_String Left(NPT_Size length) const {
        return SubString(0, length);
    }
    NPT_String Right(NPT_Size length) const {
        return length >= GetLength() ? 
               *this : 
               SubString(GetLength()-length, length);
    }

    // buffer management
    void       Reserve(NPT_Size length);

    // conversions
    NPT_String ToLowercase() const;
    NPT_String ToUppercase() const;
    NPT_Result ToInteger(long& value, bool relaxed = true) const;
    NPT_Result ToInteger(unsigned long& value, bool relaxed = true) const;
    NPT_Result ToFloat(float& value, bool relaxed = true) const;
    
    // processing
    void MakeLowercase();
    void MakeUppercase();
    void Replace(char a, char b);

    // search
    int  Find(char c, NPT_Ordinal start = 0, bool ignore_case = false) const;
    int  Find(const char* s, NPT_Ordinal start = 0, bool ignore_case = false) const;
    int  ReverseFind(char c, NPT_Ordinal start = 0, bool ignore_case = false) const;
    int  ReverseFind(const char* s, NPT_Ordinal start = 0, bool ignore_case = false) const;
    bool StartsWith(const char* s, bool ignore_case = false) const;
    bool EndsWith(const char* s, bool ignore_case = false) const;

    // editing
    void Insert(const char* s, NPT_Ordinal where = 0);
    void Erase(NPT_Ordinal start, NPT_Cardinal count = 1);
    void Replace(NPT_Ordinal start, NPT_Cardinal count, const char* s);
    void TrimLeft();
    void TrimLeft(char c);
    void TrimLeft(const char* chars);
    void TrimRight();
    void TrimRight(char c);
    void TrimRight(const char* chars);
    void Trim();
    void Trim(char c);
    void Trim(const char* chars);

    // type casting
    operator char*() const        { return m_Chars ? m_Chars: &EmptyString; }
    operator const char* () const { return m_Chars ? m_Chars: &EmptyString; }
    const char* GetChars() const  { return m_Chars ? m_Chars: &EmptyString; }
    char*       UseChars()        { return m_Chars ? m_Chars: &EmptyString; }

    // operator overloading
    NPT_String& operator=(const char* str);
    NPT_String& operator=(const NPT_String& str);
    NPT_String& operator=(char c);
    const NPT_String& operator+=(const NPT_String& s) {
        Append(s.GetChars(), s.GetLength());
        return *this;
    }
    const NPT_String& operator+=(const char* s) {
        Append(s);
        return *this;
    }
    const NPT_String& operator+=(char c) {
        Append(&c, 1);
        return *this;
    }
    char operator[](int index) const {
        NPT_ASSERT((unsigned int)index < GetLength());
        return GetChars()[index];
    }
    char& operator[](int index) {
        NPT_ASSERT((unsigned int)index < GetLength());
        return UseChars()[index];
    }

    // friend operators
    friend NPT_String operator+(const NPT_String& s1, const NPT_String& s2) {
        return s1+s2.GetChars();
    }
    friend NPT_String operator+(const NPT_String& s1, const char* s2);
    friend NPT_String operator+(const char* s1, const NPT_String& s2);
    friend NPT_String operator+(const NPT_String& s, char c);
    friend NPT_String operator+(char c, const NPT_String& s);

protected:
    // inner classes
    class Buffer {
    public:
        // class methods
        static Buffer* Allocate(NPT_Size allocated, NPT_Size length) {
            void* mem = ::operator new(sizeof(Buffer)+allocated+1);
            return new(mem) Buffer(allocated, length);
        }
        static char* Create(NPT_Size allocated, NPT_Size length=0) {
            Buffer* shared = Allocate(allocated, length);
            return shared->GetChars();
        }
        static char* Create(const char* copy) {
            NPT_Size length = StringLength(copy);
            Buffer* shared = Allocate(length, length);
            CopyString(shared->GetChars(), copy);
            return shared->GetChars();
        }
        static char* Create(const char* copy, NPT_Size length) {
            Buffer* shared = Allocate(length, length);
            CopyBuffer(shared->GetChars(), copy, length);
            shared->GetChars()[length] = '\0';
            return shared->GetChars();
        }
        static char* Create(char c, NPT_Cardinal repeat) {
            Buffer* shared = Allocate(repeat, repeat);
            char* s = shared->GetChars();
            while (repeat--) {
                *s++ = c;
            }
            *s = '\0';
            return shared->GetChars();
        }

        // methods
        char* GetChars() { 
            // return a pointer to the first char
            return reinterpret_cast<char*>(this+1); 
        }
        NPT_Size GetLength() const      { return m_Length; }
        void SetLength(NPT_Size length) { m_Length = length; }
        NPT_Size GetAllocated() const   { return m_Allocated; }

    private:
        // methods
        Buffer(NPT_Size allocated, NPT_Size length = 0) : 
            m_Length(length),
            m_Allocated(allocated) {}

        // members
        NPT_Cardinal m_Length;
        NPT_Cardinal m_Allocated;
        // the actual string data follows

    };
    
    // members
    char* m_Chars;

private:
    // friends
    friend class Buffer;

    // static members
    static char EmptyString;

    // methods
    Buffer* GetBuffer() const { 
        return reinterpret_cast<Buffer*>(m_Chars)-1;
    }
    void Reset() { 
        if (m_Chars != NULL) {
            delete GetBuffer(); 
            m_Chars = NULL;
        }
    }
    char* PrepareToWrite(NPT_Size length);
    void PrepareToAppend(NPT_Size length, NPT_Size allocate);

    // static methods
    static void CopyString(char* dst, const char* src) {
        while ((*dst++ = *src++)){}
    }
    
    static void CopyBuffer(char* dst, const char* src, NPT_Size size) {
        while (size--) *dst++ = *src++;
    }
    
    static NPT_Size StringLength(const char* str) {
        NPT_Size length = 0;
        while (*str++) length++;
        return length;
    }
};

/*----------------------------------------------------------------------
|   external operators
+---------------------------------------------------------------------*/
inline bool operator==(const NPT_String& s1, const NPT_String& s2) { 
    return s1.Compare(s2) == 0; 
}
inline bool operator==(const NPT_String& s1, const char* s2) {
    return s1.Compare(s2) == 0; 
}
inline bool operator==(const char* s1, const NPT_String& s2) {
    return s2.Compare(s1) == 0; 
}
inline bool operator!=(const NPT_String& s1, const NPT_String& s2) {
    return s1.Compare(s2) != 0; 
}
inline bool operator!=(const NPT_String& s1, const char* s2) {
    return s1.Compare(s2) != 0; 
}
inline bool operator!=(const char* s1, const NPT_String& s2) {
    return s2.Compare(s1) != 0; 
}
inline bool operator<(const NPT_String& s1, const NPT_String& s2) {
    return s1.Compare(s2) < 0; 
}
inline bool operator<(const NPT_String& s1, const char* s2) {
    return s1.Compare(s2) < 0; 
}
inline bool operator<(const char* s1, const NPT_String& s2) {
    return s2.Compare(s1) > 0; 
}
inline bool operator>(const NPT_String& s1, const NPT_String& s2) {
    return s1.Compare(s2) > 0; 
}
inline bool operator>(const NPT_String& s1, const char* s2) {
    return s1.Compare(s2) > 0; 
}
inline bool operator>(const char* s1, const NPT_String& s2) {
    return s2.Compare(s1) < 0; 
}
inline bool operator<=(const NPT_String& s1, const NPT_String& s2) {
    return s1.Compare(s2) <= 0; 
}
inline bool operator<=(const NPT_String& s1, const char* s2) {
    return s1.Compare(s2) <= 0; 
}
inline bool operator<=(const char* s1, const NPT_String& s2) {
    return s2.Compare(s1) >= 0; 
}
inline bool operator>=(const NPT_String& s1, const NPT_String& s2) {
    return s1.Compare(s2) >= 0; 
}
inline bool operator>=(const NPT_String& s1, const char* s2) {
    return s1.Compare(s2) >= 0; 
}
inline bool operator>=(const char* s1, const NPT_String& s2) {
    return s2.Compare(s1) <= 0; 
}

#endif // _NPT_STRINGS_H_
