/** @mainpage

<h1> TinyXml </h1>

TinyXml is a simple, small, C++ XML parser that can be easily 
integrating into other programs.


<h2> What it does. </h2>
	
In brief, TinyXml parses an XML document, and builds from that a 
Document Object Model that can be read, modified, and saved.

XML stands for "eXtensible Markup Language." It allows you to create 
your own document markups. Where HTML does a very good job of marking 
documents for browsers, XML allows you to define any kind of document 
markup, for example a document that describes a "to do" list for an 
organizer application. XML is a very structured and convenient format.
All those random file formats created to store application data can 
all be replaced with XML. One parser for everything.

There are different ways to access and interact with XML data.
TinyXml uses a Document Object Model, meaning the XML data is parsed
into a tree objects that can be browsed and manipulated, and then 
written back to disk. You can also construct an XML document from 
scratch with C++ objects and write this to disk.

TinyXml is designed to be easy and fast. It is one header and three cpp 
files. Simply add these to your project and off you go. There is an 
example to get you started. It is released under the ZLib license, 
so you can use it in open source or commercial code.

It attempts to be a flexible parser, but with truly correct and 
compliant XML output (with the exception of the character set,
below.) TinyXml should compile on any reasonably C++ 
system. It does not rely on exceptions or RTTI, and only uses the STL
string class.


<h2> What it doesn't do. </h2>

It doesn’t parse or use DTDs (Document Type Definitions) or XSL’s 
(eXtensible Stylesheet Language.) It is limited to the ASCII 
character set. There are other parsers out there (check out 
www.sourceforge.org, search for XML) that are much more fully 
featured. But they are also much bigger, take longer to set up in 
your project, have a higher learning curve, and often have a more 
restrictive license. If you are working with browsers or have more 
complete XML needs, TinyXml is not the parser for you. 


<h2> Code Status.  </h2>

Currently in use, TinyXml is looking pretty stable. If you find
bugs, send them in and we'll get them straightened out as soon as possible.

There are some areas of improvement; please check sourceforge if you are 
interested in working on TinxXml.


<h2> Changes between version 1 and 2 </h2>


<h3> Entities </h3>
TinyXml recognizes the pre-defined "entity references", meaning special 
characters. Namely:

@verbatim
	&amp;	&
	&lt;	<
	&gt;	>
	&quot;	"
	&apos;	‘
@endverbatim

These are recognized when the XML document is read, and translated to there
ASCII equivalents. For instance, text with the XML of:

@verbatim
	Far &amp; Away
@endverbatim

will have the Value() of "Far & Away" when queried from the TiXmlText object, 
but will be written back to the XML stream/file as an entitity.

TiXml will ignore unknown entities and the 
@verbatim
"&#x"
@endverbatim
entities, and leave them unprocessed.


<h3> Streams </h3>
TiXml has been modified to support both C (FILE) and C++ (operator <<,>>) 
streams. There are some differences that you may need to be aware of.

C style output:
	- based on FILE*
	- the Print() and SaveFile() methods

	Generates formatted output, with plenty of white space, intended to be as 
	human-readable as possible. They are very fast, and tolerant of ill formed 
	XML documents. For example, an XML document that contains 2 root elements 
	and 2 declarations, will print.

C style input:
	- based on FILE*
	- the Parse() and LoadFile() methods

	A fast, tolerant read. Use whenever you don't need the C++ streams.

C++ style ouput:
	- based on std::ostream
	- operator<<

	Generates condensed output, intended for network transmission rather than
	readability. Depending on your system's implementation of the ostream class,
	these may be somewhat slower. (Or may not.) Not tolerant of ill formed XML:
	a document should contain the correct one root element. Additional root level
	elements will not be streamed out.

C++ style input:
	- based on std::istream
	- operator>>

	Reads XML from a stream, making it useful for network transmission. The tricky
	part is knowing when the XML document is complete, since there will almost
	certainly be other data in the stream. TinyXml will assume the XML data is
	complete after it reads the root element. Also not that operator>> is somewhat
	slower than Parse, due to both implementation of the STL and limitations of
	TinyXml.

<h3> White space </h3>
The world simply does not agree on whether white space should be kept, or condensed.
For example, pretend the '_' is a space, and look at "Hello____world". HTML, and 
at least some XML parsers, will interpret this as "Hello_world". They condense white
space. Some XML parsers do not, and will leave it as "Hello____world". (Remember
to keep pretending the _ is a space.)

It's an issue that hasn't been resolved to my satisfaction. TinyXml supports both
motifs. Call TiXmlBase::SetCondenseWhiteSpace( bool ) to set the desired behavior.
The default is to condense white space.

If you change the default, you should call TiXmlBase::SetCondenseWhiteSpace( bool )
before making any calls to Parse XML data, and I don't recommend changing it after
it has been set.


<h2> Using and Installing </h2>

To Compile and Run xmltest:

A Linux Makefile and a Windows Visual C++ .dsp file is provided. 
Simply compile and run. It will write the file demotest.xml to your 
disk and generate output on the screen. It also tests walking the
DOM by printing out the number of nodes found using different 
techniques.

The Linux makefile is very generic and will
probably run on other systems, but is only tested on Linux. You no
longer need to run 'make depend'. The dependecies have been
hard coded.


To Use in an Application:

Add tinyxml.cpp, tinyxml.h, tinyxmlerror.cpp, and tinyxmlparser.cpp to your 
project or make file. That's it! It should compile on any reasonably
compliant C++ system. You do not need to enable exceptions or
RTTI for TinyXml.


<h2> Where it may go.  </h2>

At this point, I'm focusing on tightening up remaining issues.
Bug fixes (though comfortably rare) and minor interface 
corrections. 

There are some "it would be nice if..." items. I'll keep those
posted as tasks on SourceForge. (www.sourceforge.net/projects/tinyxml)


<h2> How TinyXml works.  </h2>

An example is probably the best way to go. Take:
@verbatim
	<?xml version="1.0" standalone=‘no’>
	<?-- Our to do list data -->
	<ToDo>
		<Item priority="1"> Go to the <bold>Toy store!</bold></Item>
		<Item priority="2"> Do bills</Item>
	</ToDo>
@endverbatim

It’s not much of a To Do list, but it will do. To read this file 
(say "demo.xml") you would create a document, and parse it in:
@verbatim
	TiXmlDocument doc( "demo.xml" );
	doc.LoadFile();
@endverbatim

And it’s ready to go. Now let’s look at some lines and how they 
relate to the DOM.

<?xml version="1.0" standalone=‘no’>

	The first line is a declaration, and gets turned into the
	TiXmlDeclaration class. It will be the first child of the
	document node.
	
	This is the only directive/special tag parsed by by TinyXml.
	Generally directive targs are stored in TiXmlUnknown so the 
	commands won’t be lost when it is saved back to disk.

<?-- Our to do list data -->

	A comment. Will become a TiXmlComment object.

<ToDo>

	The ToDo tag defines a TiXmlElement object. This one does not have 
	any attributes, but will contain 2 other elements, both of which 
	are items.

<Item priority="1"> 

	Creates another TiXmlElement which is a child of the "ToDo" element. 
	This element has 1 attribute, with the name ‘priority’ and the value 
	‘1’.

Go to the 

	A TiXmlText. This is a leaf node and cannot contain other nodes. 
	It is a child of the ‘Item" Element.

<bold>
	
	Another TiXmlElement, this one a child of the "Item" element.

Etc.

Looking at the entire object tree, you end up with:
@verbatim
TiXmlDocument				"demo.xml"
	TiXmlDeclaration		"version='1.0'" "standalone=‘no’"
	TiXmlComment			" Our to do list data"
	TiXmlElement			"ToDo"
		TiXmlElement		"Item"		Attribtutes: priority = 1
			TiXmlText		"Go to the "
			TiXmlElement    "bold"
				TiXmlText	"Toy store!"
	TiXmlElement			"Item"		Attributes: priority=2
		TiXmlText			"bills"
@endverbatim

<h2> Contributors </h2>

Thanks very much to everyone who sends suggestions, bugs, ideas, and 
encouragement. It all helps, and makes this project fun. A special thanks
to the contributors on the web pages that keep it lively.

So many people have sent in bugs and ideas, that rather than list here I
try to give credit due in the "changes.txt" file.

<h2> Documentation </h2>

The documentation is build with Doxygen, using the 'dox' 
configuration file.

<h2> License </h2>

TinyXml is released under the zlib license:

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

<h2> References  </h2>

The World Wide Web Consortium is the definitive standard body for 
XML, and there web pages contain huge amounts of information. I also 
recommend "XML Pocket Reference" by Robert Eckstein and published by 
O’Reilly.

<h2> Contact Me: </h2>

I’d appreciates your suggestions, and would love to know if you 
use TinyXml. I hope you enjoy it and find it useful. Please post
questions, comments, file bugs, or contact me at:

www.sourceforge.net/projects/tinyxml

Lee Thomason
*/
