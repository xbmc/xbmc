//  g++ -Wall -O2 contrib/html5-printer.cpp -o html5-printer -ltinyxml2

//  This program demonstrates how to use "tinyxml2" to generate conformant HTML5
//  by deriving from the "tinyxml2::XMLPrinter" class.

//  http://dev.w3.org/html5/markup/syntax.html

//  In HTML5, there are 16 so-called "void" elements.  "void elements" NEVER have
//  inner content (but they MAY have attributes), and are assumed to be self-closing.
//  An example of a self-closig HTML5 element is "<br/>" (line break)
//  All other elements are called "non-void" and MUST never self-close.
//  Examples: "<div class='lolcats'></div>".

//  tinyxml2::XMLPrinter will emit _ALL_ XML elements with no inner content as
//  self-closing.  This behavior produces space-effeceint XML, but incorrect HTML5.

//  Author: Dennis Jenkins,  dennis (dot) jenkins (dot) 75 (at) gmail (dot) com.
//  License: Same as tinyxml2 (zlib, see below)
//  This example is a small contribution to the world!  Enjoy it!

/*
This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/


#include <tinyxml2.h>
#include <iostream>

#if defined (_MSC_VER)
#define strcasecmp stricmp
#endif

using namespace tinyxml2;

// Contrived input containing a mix of void and non-void HTML5 elements.
// When printed via XMLPrinter, some non-void elements will self-close (not valid HTML5).
static const char input[] =
"<html><body><p style='a'></p><br/>&copy;<col a='1' b='2'/><div a='1'></div></body></html>";

// XMLPrinterHTML5 is small enough, just put the entire implementation inline.
class	XMLPrinterHTML5 : public XMLPrinter
{
public:
    XMLPrinterHTML5 (FILE* file=0, bool compact = false, int depth = 0) :
        XMLPrinter (file, compact, depth)
    {}

protected:
    virtual void CloseElement () {
        if (_elementJustOpened && !isVoidElement (_stack.PeekTop())) {
            SealElement();
            }
        XMLPrinter::CloseElement();
    }

    virtual bool isVoidElement (const char *name) {
// Complete list of all HTML5 "void elements",
// http://dev.w3.org/html5/markup/syntax.html
        static const char *list[] = {
            "area", "base", "br", "col", "command", "embed", "hr", "img",
            "input", "keygen", "link", "meta", "param", "source", "track", "wbr",
            NULL
        };

// I could use 'bsearch', but I don't have MSVC to test on (it would work with gcc/libc).
        for (const char **p = list; *p; ++p) {
            if (!strcasecmp (name, *p)) {
                return true;
            }
        }

        return false;
    }
};

int	main (void) {
    XMLDocument doc (false);
    doc.Parse (input);

    std::cout << "INPUT:\n" << input << "\n\n";

    XMLPrinter prn (NULL, true);
    doc.Print (&prn);
    std::cout << "XMLPrinter (not valid HTML5):\n" << prn.CStr() << "\n\n";

    XMLPrinterHTML5 html5 (NULL, true);
    doc.Print (&html5);
    std::cout << "XMLPrinterHTML5:\n" << html5.CStr() << "\n";

    return 0;
}
