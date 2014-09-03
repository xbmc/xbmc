TinyXML-2
=========
![TinyXML-2 Logo](http://www.grinninglizard.com/tinyxml2/TinyXML2_small.png)

TinyXML-2 is a simple, small, efficient, C++ XML parser that can be 
easily integrated into other programs.

The master is hosted on github:
https://github.com/leethomason/tinyxml2

The online HTML version of these docs:
http://grinninglizard.com/tinyxml2docs/index.html

Examples are in the "related pages" tab of the HTML docs.

What it does.
-------------
	
In brief, TinyXML-2 parses an XML document, and builds from that a 
Document Object Model (DOM) that can be read, modified, and saved.

XML stands for "eXtensible Markup Language." It is a general purpose
human and machine readable markup language to describe arbitrary data.
All those random file formats created to store application data can 
all be replaced with XML. One parser for everything.

http://en.wikipedia.org/wiki/XML

There are different ways to access and interact with XML data.
TinyXML-2 uses a Document Object Model (DOM), meaning the XML data is parsed
into a C++ objects that can be browsed and manipulated, and then 
written to disk or another output stream. You can also construct an XML document 
from scratch with C++ objects and write this to disk or another output
stream. You can even use TinyXML-2 to stream XML programmatically from
code without creating a document first.

TinyXML-2 is designed to be easy and fast to learn. It is one header and
one cpp file. Simply add these to your project and off you go. 
There is an example file - xmltest.cpp - to get you started. 

TinyXML-2 is released under the ZLib license, 
so you can use it in open source or commercial code. The details
of the license are at the top of every source file.

TinyXML-2 attempts to be a flexible parser, but with truly correct and
compliant XML output. TinyXML-2 should compile on any reasonably C++
compliant system. It does not rely on exceptions, RTTI, or the STL.

What it doesn't do.
-------------------

TinyXML-2 doesn't parse or use DTDs (Document Type Definitions) or XSLs
(eXtensible Stylesheet Language.) There are other parsers out there 
that are much more fully featured. But they are also much bigger, 
take longer to set up in your project, have a higher learning curve, 
and often have a more restrictive license. If you are working with 
browsers or have more complete XML needs, TinyXML-2 is not the parser for you.

TinyXML-1 vs. TinyXML-2
-----------------------

TinyXML-2 is now the focus of all development, well tested, and your
best choice unless you have a requirement to maintain TinyXML-1 code.

TinyXML-2 uses a similar API to TinyXML-1 and the same
rich test cases. But the implementation of the parser is completely re-written
to make it more appropriate for use in a game. It uses less memory, is faster,
and uses far fewer memory allocations.

TinyXML-2 has no requirement for STL, but has also dropped all STL support. All
strings are query and set as 'const char*'. This allows the use of internal 
allocators, and keeps the code much simpler.

Both parsers:

1.  Simple to use with similar APIs.
2.  DOM based parser.
3.  UTF-8 Unicode support. http://en.wikipedia.org/wiki/UTF-8

Advantages of TinyXML-2

1.  The focus of all future dev.
2.  Many fewer memory allocation (1/10th to 1/100th), uses less memory
    (about 40% of TinyXML-1), and faster.
3.  No STL requirement.
4.  More modern C++, including a proper namespace.
5.  Proper and useful handling of whitespace

Advantages of TinyXML-1

1.  Can report the location of parsing errors.
2.  Support for some C++ STL conventions: streams and strings
3.  Very mature and well debugged code base.

Features
--------

### Memory Model

An XMLDocument is a C++ object like any other, that can be on the stack, or
new'd and deleted on the heap.

However, any sub-node of the Document, XMLElement, XMLText, etc, can only
be created by calling the appropriate XMLDocument::NewElement, NewText, etc.
method. Although you have pointers to these objects, they are still owned
by the Document. When the Document is deleted, so are all the nodes it contains.

### White Space

#### Whitespace Preservation (default)

Microsoft has an excellent article on white space: http://msdn.microsoft.com/en-us/library/ms256097.aspx

By default, TinyXML-2 preserves white space in a (hopefully) sane way that is almost complient with the
spec. (TinyXML-1 used a completely different model, much more similar to 'collapse', below.)

As a first step, all newlines / carriage-returns / line-feeds are normalized to a
line-feed character, as required by the XML spec.

White space in text is preserved. For example:

	<element> Hello,  World</element>

The leading space before the "Hello" and the double space after the comma are 
preserved. Line-feeds are preserved, as in this example:

	<element> Hello again,  
	          World</element>

However, white space between elements is **not** preserved. Although not strictly 
compliant, tracking and reporting inter-element space is awkward, and not normally
valuable. TinyXML-2 sees these as the same XML:

	<document>
		<data>1</data>
		<data>2</data>
		<data>3</data>
	</document>

	<document><data>1</data><data>2</data><data>3</data></document>

#### Whitespace Collapse

For some applications, it is preferable to collapse whitespace. Collapsing
whitespace gives you "HTML-like" behavior, which is sometimes more suitable
for hand typed documents. 

TinyXML-2 supports this with the 'whitespace' parameter to the XMLDocument constructor.
(The default is to preserve whitespace, as described above.)

However, you may also use COLLAPSE_WHITESPACE, which will:

* Remove leading and trailing whitespace
* Convert newlines and line-feeds into a space character
* Collapse a run of any number of space characters into a single space character

Note that (currently) there is a performance impact for using COLLAPSE_WHITESPACE.
It essentially causes the XML to be parsed twice.

### Entities

TinyXML-2 recognizes the pre-defined "character entities", meaning special
characters. Namely:

	&amp;	&
	&lt;	<
	&gt;	>
	&quot;	"
	&apos;	'

These are recognized when the XML document is read, and translated to their
UTF-8 equivalents. For instance, text with the XML of:

	Far &amp; Away

will have the Value() of "Far & Away" when queried from the XMLText object,
and will be written back to the XML stream/file as an ampersand. 

Additionally, any character can be specified by its Unicode code point:
The syntax "&#xA0;" or "&#160;" are both to the non-breaking space characher. 
This is called a 'numeric character reference'. Any numeric character reference
that isn't one of the special entities above, will be read, but written as a
regular code point. The output is correct, but the entity syntax isn't preserved.

### Printing

#### Print to file
You can directly use the convenience function:

	XMLDocument doc;
	...
	doc.SaveFile( "foo.xml" );

Or the XMLPrinter class:

	XMLPrinter printer( fp );
	doc.Print( &printer );

#### Print to memory
Printing to memory is supported by the XMLPrinter.

	XMLPrinter printer;
	doc.Print( &printer );
	// printer.CStr() has a const char* to the XML

#### Print without an XMLDocument

When loading, an XML parser is very useful. However, sometimes
when saving, it just gets in the way. The code is often set up
for streaming, and constructing the DOM is just overhead.

The Printer supports the streaming case. The following code
prints out a trivially simple XML file without ever creating
an XML document.

	XMLPrinter printer( fp );
	printer.OpenElement( "foo" );
	printer.PushAttribute( "foo", "bar" );
	printer.CloseElement();

Examples
--------

#### Load and parse an XML file.

	/* ------ Example 1: Load and parse an XML file. ---- */	
	{
		XMLDocument doc;
		doc.LoadFile( "dream.xml" );
	}

#### Lookup information.

	/* ------ Example 2: Lookup information. ---- */	
	{
		XMLDocument doc;
		doc.LoadFile( "dream.xml" );

		// Structure of the XML file:
		// - Element "PLAY"      the root Element, which is the 
		//                       FirstChildElement of the Document
		// - - Element "TITLE"   child of the root PLAY Element
		// - - - Text            child of the TITLE Element
		
		// Navigate to the title, using the convenience function,
		// with a dangerous lack of error checking.
		const char* title = doc.FirstChildElement( "PLAY" )->FirstChildElement( "TITLE" )->GetText();
		printf( "Name of play (1): %s\n", title );
		
		// Text is just another Node to TinyXML-2. The more
		// general way to get to the XMLText:
		XMLText* textNode = doc.FirstChildElement( "PLAY" )->FirstChildElement( "TITLE" )->FirstChild()->ToText();
		title = textNode->Value();
		printf( "Name of play (2): %s\n", title );
	}

Using and Installing
--------------------

There are 2 files in TinyXML-2:
* tinyxml2.cpp
* tinyxml2.h

And additionally a test file:
* xmltest.cpp

Simply compile and run. There is a visual studio 2010 project included, a simple Makefile, 
an XCode project, a Code::Blocks project, and a cmake CMakeLists.txt included to help you. 
The top of tinyxml.h even has a simple g++ command line if you are are *nix and don't want 
to use a build system.

Versioning
----------

TinyXML-2 uses semantic versioning. http://semver.org/ Releases are now tagged in github.

Note that the major version will (probably) change fairly rapidly. API changes are fairly
common.

Documentation
-------------

The documentation is build with Doxygen, using the 'dox' 
configuration file.

License
-------

TinyXML-2 is released under the zlib license:

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

Contributors
------------

Thanks very much to everyone who sends suggestions, bugs, ideas, and 
encouragement. It all helps, and makes this project fun.

The original TinyXML-1 has many contributors, who all deserve thanks
in shaping what is a very successful library. Extra thanks to Yves
Berquin and Andrew Ellerton who were key contributors.

TinyXML-2 grew from that effort. Lee Thomason is the original author
of TinyXML-2 (and TinyXML-1) but TinyXML-2 has been and is being improved
by many contributors.

Thanks to John Mackay at http://john.mackay.rosalilastudio.com for the TinyXML-2 logo!


