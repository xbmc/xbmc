/*
Copyright (c) 2000 Lee Thomason (www.grinninglizard.com)

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


#ifndef TINYXML_INCLUDED
#define TINYXML_INCLUDED

#pragma warning( disable : 4530 )
#pragma warning( disable : 4786 )

#include <string>
#include <stdio.h>
#include <assert.h>

class TiXmlDocument;
class TiXmlElement;
class TiXmlComment;
class TiXmlUnknown;
class TiXmlAttribute;
class TiXmlText;
class TiXmlDeclaration;


// Help out windows:
#if defined( _DEBUG ) && !defined( DEBUG )
	#define DEBUG
#endif

#if defined( DEBUG ) && defined( _MSC_VER )
	#include <windows.h>
	#define TIXML_LOG OutputDebugString
#else
	#define TIXML_LOG printf
#endif 


/** TiXmlBase is a base class for every class in TinyXml.
	It does little except to establish that TinyXml classes
	can be printed and provide some utility functions.

	In XML, the document and elements can contain
	other elements and other types of nodes.

	@verbatim
	A Document can contain:	Element	(container or leaf)
							Comment (leaf)
							Unknown (leaf)
							Declaration( leaf )

	An Element can contain:	Element (container or leaf)
							Text	(leaf)
							Attributes (not on tree)
							Comment (leaf)
							Unknown (leaf)

	A Decleration contains: Attributes (not on tree)
	@endverbatim
*/
class TiXmlBase
{
	friend class TiXmlNode;
	friend class TiXmlElement;
	friend class TiXmlDocument;
 
  public:
	TiXmlBase()								{}	
	virtual ~TiXmlBase()					{}
	
	/**	All TinyXml classes can print themselves to a filestream.
		This is a formatted print, and will insert tabs and newlines.
		
		(For an unformatted stream, use the << operator.)
	*/
 	virtual void Print( FILE* cfile, int depth ) const = 0;

	// [internal] Underlying implementation of the operator <<
	virtual void StreamOut ( std::ostream* out ) const = 0;

	/**	The world does not agree on whether white space should be kept or
		not. In order to make everyone happy, these global, static functions
		are provided to set whether or not TinyXml will condense all white space
		into a single space or not. The default is to condense. Note changing these
		values is not thread safe.
	*/
	static void SetCondenseWhiteSpace( bool condense )		{ condenseWhiteSpace = condense; }

	/// Return the current white space setting.
	static bool IsWhiteSpaceCondensed()						{ return condenseWhiteSpace; }

  protected:
	static const char* SkipWhiteSpace( const char* );
	static bool StreamWhiteSpace( std::istream* in, std::string* tag );
	static bool IsWhiteSpace( int c )		{ return ( isspace( c ) || c == '\n' || c == '\r' ); }

	/*	Read to the specified character.
		Returns true if the character found and no error.
	*/
	static bool StreamTo( std::istream* in, int character, std::string* tag );

	/*	Reads an XML name into the string provided. Returns
		a pointer just past the last character of the name, 
		or 0 if the function has an error.
	*/
	static const char* ReadName( const char*, std::string* name );

	/*	Reads text. Returns a pointer past the given end tag.
		Wickedly complex options, but it keeps the (sensitive) code in one place.
	*/
	static const char* ReadText(	const char* in,				// where to start
									std::string* text,			// the string read
									bool ignoreWhiteSpace,		// whether to keep the white space
									const char* endTag,			// what ends this text
									bool ignoreCase );			// whether to ignore case in the end tag

	virtual const char* Parse( const char* p ) = 0;

	// If an entity has been found, transform it into a character.
	static const char* GetEntity( const char* in, char* value );

	// Get a character, while interpreting entities.
	inline static const char* GetChar( const char* p, char* value )		
											{		
												assert( p );
												if ( *p == '&' ) 
												{
													return GetEntity( p, value );
												}
												else 
												{
													*value = *p;
													return p+1;
												}
											}

	// Puts a string to a stream, expanding entities as it goes.
	// Note this should not contian the '<', '>', etc, or they will be transformed into entities!
	static void PutString( const std::string& str, std::ostream* stream );

	// Return true if the next characters in the stream are any of the endTag sequences.
	bool static StringEqual(	const char* p, 
								const char* endTag, 
								bool ignoreCase );
													

	enum
	{
		TIXML_NO_ERROR = 0,
		TIXML_ERROR,
		TIXML_ERROR_OPENING_FILE,
		TIXML_ERROR_OUT_OF_MEMORY,
		TIXML_ERROR_PARSING_ELEMENT,
		TIXML_ERROR_FAILED_TO_READ_ELEMENT_NAME,
		TIXML_ERROR_READING_ELEMENT_VALUE,
		TIXML_ERROR_READING_ATTRIBUTES,
		TIXML_ERROR_PARSING_EMPTY,
		TIXML_ERROR_READING_END_TAG,
		TIXML_ERROR_PARSING_UNKNOWN,
		TIXML_ERROR_PARSING_COMMENT,
		TIXML_ERROR_PARSING_DECLARATION,
		TIXML_ERROR_DOCUMENT_EMPTY,

		TIXML_ERROR_STRING_COUNT
	};
	static const char* errorString[ TIXML_ERROR_STRING_COUNT ];

  private:
	struct Entity
	{
		const char*     str;
		unsigned int	strLength;
		int			    chr;
	};
	enum
	{
		NUM_ENTITY = 5,
		MAX_ENTITY_LENGTH = 6

	};
	static Entity entity[ NUM_ENTITY ];
	static bool condenseWhiteSpace;
};


/** The parent class for everything in the Document Object Model.
	(Except for attributes, which are contained in elements.)
	Nodes have siblings, a parent, and children. A node can be
	in a document, or stand on its own. The type of a TiXmlNode
	can be queried, and it can be cast to its more defined type.
*/
class TiXmlNode : public TiXmlBase
{
  public:

	/** An output stream operator, for every class. Note that this outputs
		without any newlines or formatting, as opposed to Print(), which
		includes tabs and new lines.

		The operator<< and operator>> are not completely symmetric. Writing
		a node to a stream is very well defined. You'll get a nice stream
		of output, without any extra whitespace or newlines. 
		
		But reading is not as well defined. (As it always is.) If you create
		a TiXmlElement (for example) and read that from an input stream,
		the text needs to define an element or junk will result. This is
		true of all input streams, but it's worth keeping in mind.

		A TiXmlDocument will read nodes until it reads a root element.
	*/
	friend std::ostream& operator<< ( std::ostream& out, const TiXmlNode& base )
	{
		base.StreamOut( &out );
		return out;
	}

	/** An input stream operator, for every class. Tolerant of newlines and
		formatting, but doesn't expect them.
	*/
	friend std::istream& operator>> ( std::istream& in, TiXmlNode& base )
	{
		std::string tag;
		tag.reserve( 8 * 1000 );
		base.StreamIn( &in, &tag );
		
		base.Parse( tag.c_str() );
		return in;
	}

	/** The types of XML nodes supported by TinyXml. (All the
		unsupported types are picked up by UNKNOWN.)
	*/
	enum NodeType 
	{
		DOCUMENT, 
		ELEMENT, 
		COMMENT, 
		UNKNOWN, 
		TEXT, 
		DECLARATION, 
		TYPECOUNT
	};

	virtual ~TiXmlNode();

	/** The meaning of 'value' changes for the specific type of
		TiXmlNode.
		@verbatim
		Document:	filename of the xml file
		Element:	name of the element
		Comment:	the comment text
		Unknown:	the tag contents
		Text:		the text string
		@endverbatim

		The subclasses will wrap this function.
	*/
	const std::string& Value()	const			{ return value; }

	/** Changes the value of the node. Defined as:
		@verbatim
		Document:	filename of the xml file
		Element:	name of the element
		Comment:	the comment text
		Unknown:	the tag contents
		Text:		the text string
		@endverbatim
	*/
	void SetValue( const std::string& _value )		{ value = _value; }

	/// Delete all the children of this node. Does not affect 'this'.
	void Clear();

	/// One step up the DOM.
	TiXmlNode* Parent() const					{ return parent; }

	TiXmlNode* FirstChild()	const	{ return firstChild; }		///< The first child of this node. Will be null if there are no children.
	TiXmlNode* FirstChild( const std::string& value ) const;	///< The first child of this node with the matching 'value'. Will be null if none found.
	
	TiXmlNode* LastChild() const	{ return lastChild; }		/// The last child of this node. Will be null if there are no children.
	TiXmlNode* LastChild( const std::string& value ) const;		/// The last child of this node matching 'value'. Will be null if there are no children.

	/** An alternate way to walk the children of a node.
		One way to iterate over nodes is:
		@verbatim
			for( child = parent->FirstChild(); child; child = child->NextSibling() )
		@endverbatim

		IterateChildren does the same thing with the syntax:
		@verbatim
			child = 0;
			while( child = parent->IterateChildren( child ) )
		@endverbatim

		IterateChildren takes the previous child as input and finds
		the next one. If the previous child is null, it returns the
		first. IterateChildren will return null when done.
	*/
	TiXmlNode* IterateChildren( TiXmlNode* previous ) const;

	/// This flavor of IterateChildren searches for children with a particular 'value'
	TiXmlNode* IterateChildren( const std::string& value, TiXmlNode* previous ) const;
		
	/** Add a new node related to this. Adds a child past the LastChild.
		Returns a pointer to the new object or NULL if an error occured.
	*/
	TiXmlNode* InsertEndChild( const TiXmlNode& addThis );					

	/** Add a new node related to this. Adds a child before the specified child.
		Returns a pointer to the new object or NULL if an error occured.
	*/
	TiXmlNode* InsertBeforeChild( TiXmlNode* beforeThis, const TiXmlNode& addThis );

	/** Add a new node related to this. Adds a child after the specified child.
		Returns a pointer to the new object or NULL if an error occured.
	*/
	TiXmlNode* InsertAfterChild(  TiXmlNode* afterThis, const TiXmlNode& addThis );
	
	/** Replace a child of this node.
		Returns a pointer to the new object or NULL if an error occured.
	*/
	TiXmlNode* ReplaceChild( TiXmlNode* replaceThis, const TiXmlNode& withThis );
	
	/// Delete a child of this node.
	bool RemoveChild( TiXmlNode* removeThis );

	/// Navigate to a sibling node.
	TiXmlNode* PreviousSibling() const			{ return prev; }

	/// Navigate to a sibling node.
	TiXmlNode* PreviousSibling( const std::string& ) const;
	
	/// Navigate to a sibling node.
	TiXmlNode* NextSibling() const				{ return next; }

	/// Navigate to a sibling node with the given 'value'.
	TiXmlNode* NextSibling( const std::string& ) const;

	/** Convenience function to get through elements. 
		Calls NextSibling and ToElement. Will skip all non-Element
		nodes. Returns 0 if there is not another element.
	*/
	TiXmlElement* NextSiblingElement() const;

	/** Convenience function to get through elements. 
		Calls NextSibling and ToElement. Will skip all non-Element
		nodes. Returns 0 if there is not another element.
	*/
	TiXmlElement* NextSiblingElement( const std::string& ) const;

	/// Convenience function to get through elements.
	TiXmlElement* FirstChildElement()	const;
	
	/// Convenience function to get through elements.
	TiXmlElement* FirstChildElement( const std::string& value ) const;

	/// Query the type (as an enumerated value, above) of this node.
	virtual int Type() const	{ return type; }

	/** Return a pointer to the Document this node lives in. 
		Returns null if not in a document.
	*/
	TiXmlDocument* GetDocument() const;

	/// Returns true if this node has no children.
	bool NoChildren() const						{ return !firstChild; }

	TiXmlDocument* ToDocument()	const		{ return ( this && type == DOCUMENT ) ? (TiXmlDocument*) this : 0; } ///< Cast to a more defined type. Will return null not of the requested type.
	TiXmlElement*  ToElement() const		{ return ( this && type == ELEMENT  ) ? (TiXmlElement*)  this : 0; } ///< Cast to a more defined type. Will return null not of the requested type.
	TiXmlComment*  ToComment() const		{ return ( this && type == COMMENT  ) ? (TiXmlComment*)  this : 0; } ///< Cast to a more defined type. Will return null not of the requested type.
	TiXmlUnknown*  ToUnknown() const		{ return ( this && type == UNKNOWN  ) ? (TiXmlUnknown*)  this : 0; } ///< Cast to a more defined type. Will return null not of the requested type.
	TiXmlText*	   ToText()    const		{ return ( this && type == TEXT     ) ? (TiXmlText*)     this : 0; } ///< Cast to a more defined type. Will return null not of the requested type.
	TiXmlDeclaration* ToDeclaration() const	{ return ( this && type == DECLARATION ) ? (TiXmlDeclaration*) this : 0; } ///< Cast to a more defined type. Will return null not of the requested type.

	virtual TiXmlNode* Clone() const = 0;

	// The real work of the input operator.
	virtual void StreamIn( std::istream* in, std::string* tag ) = 0;

  protected:
	TiXmlNode( NodeType type );

	// The node is passed in by ownership. This object will delete it.
	TiXmlNode* LinkEndChild( TiXmlNode* addThis );

	// Figure out what is at *p, and parse it. Returns null if it is not an xml node.
	TiXmlNode* Identify( const char* start );

	void CopyToClone( TiXmlNode* target ) const	{ target->value = value; }

	TiXmlNode*		parent;		
	NodeType		type;
	
	TiXmlNode*		firstChild;
	TiXmlNode*		lastChild;

	std::string		value;
	
	TiXmlNode*		prev;
	TiXmlNode*		next;
};


/** An attribute is a name-value pair. Elements have an arbitrary
	number of attributes, each with a unique name.

	@note The attributes are not TiXmlNodes, since they are not
		  part of the tinyXML document object model. There are other
		  suggested ways to look at this problem.

	@note Attributes have a parent
*/
class TiXmlAttribute : public TiXmlBase
{
	friend class TiXmlAttributeSet;

  public:
	/// Construct an empty attribute.
	TiXmlAttribute() : prev( 0 ), next( 0 )	{}

	/// Construct an attribute with a name and value.
	TiXmlAttribute( const std::string& _name, const std::string& _value )	: name( _name ), value( _value ), prev( 0 ), next( 0 ) {}

	const std::string& Name()  const { return name; }		///< Return the name of this attribute.

	const std::string& Value() const { return value; }		///< Return the value of this attribute.
	const int          IntValue() const;					///< Return the value of this attribute, converted to an integer.
	const double	   DoubleValue() const;					///< Return the value of this attribute, converted to a double.

	void SetName( const std::string& _name )	{ name = _name; }		///< Set the name of this attribute.
	void SetValue( const std::string& _value )	{ value = _value; }		///< Set the value.
	void SetIntValue( int value );										///< Set the value from an integer.
	void SetDoubleValue( double value );								///< Set the value from a double.

	/// Get the next sibling attribute in the DOM. Returns null at end.
	TiXmlAttribute* Next() const;
	/// Get the previous sibling attribute in the DOM. Returns null at beginning.
	TiXmlAttribute* Previous() const;

	bool operator==( const TiXmlAttribute& rhs ) const { return rhs.name == name; }
	bool operator<( const TiXmlAttribute& rhs )	 const { return name < rhs.name; }
	bool operator>( const TiXmlAttribute& rhs )  const { return name > rhs.name; }

	/*	[internal use] 
		Attribtue parsing starts: first letter of the name
						 returns: the next char after the value end quote
	*/	
	virtual const char* Parse( const char* p );

	// [internal use] 
 	virtual void Print( FILE* cfile, int depth ) const;

	// [internal use] 
	virtual void StreamOut( std::ostream* out ) const;

	// [internal use]
	// Set the document pointer so the attribute can report errors.
	void SetDocument( TiXmlDocument* doc )	{ document = doc; }

  private:
	TiXmlDocument*	document;	// A pointer back to a document, for error reporting.
	std::string		name;
	std::string		value;

	TiXmlAttribute*	prev;
	TiXmlAttribute*	next;
};


/*	A class used to manage a group of attributes.
	It is only used internally, both by the ELEMENT and the DECLARATION.
	
	The set can be changed transparent to the Element and Declaration
	classes that use it, but NOT transparent to the Attribute 
	which has to implement a next() and previous() method. Which makes
	it a bit problematic and prevents the use of STL.

	This version is implemented with circular lists because:
		- I like circular lists
		- it demonstrates some independence from the (typical) doubly linked list.
*/
class TiXmlAttributeSet
{
  public:
	TiXmlAttributeSet();
	~TiXmlAttributeSet();

	void Add( TiXmlAttribute* attribute );
	void Remove( TiXmlAttribute* attribute );

	TiXmlAttribute* First() const	{ return ( sentinel.next == &sentinel ) ? 0 : sentinel.next; }
	TiXmlAttribute* Last()  const	{ return ( sentinel.prev == &sentinel ) ? 0 : sentinel.prev; }
	
	TiXmlAttribute*	Find( const std::string& name ) const;

  private:
	TiXmlAttribute sentinel;
};


/** The element is a container class. It has a value, the element name,
	and can contain other elements, text, comments, and unknowns.
	Elements also contain an arbitrary number of attributes.
*/
class TiXmlElement : public TiXmlNode
{
  public:
	/// Construct an element.
	TiXmlElement( const std::string& value );

	virtual ~TiXmlElement();

	/** Given an attribute name, attribute returns the value
		for the attribute of that name, or null if none exists.
	*/
	const std::string* Attribute( const std::string& name ) const;

	/** Given an attribute name, attribute returns the value
		for the attribute of that name, or null if none exists.
	*/
	const std::string* Attribute( const std::string& name, int* i ) const;

	/** Sets an attribute of name to a given value. The attribute
		will be created if it does not exist, or changed if it does.
	*/
	void SetAttribute( const std::string& name, 
					   const std::string& value );

	/** Sets an attribute of name to a given value. The attribute
		will be created if it does not exist, or changed if it does.
	*/
	void SetAttribute( const std::string& name, 
					   int value );

	/** Deletes an attribute with the given name.
	*/
	void RemoveAttribute( const std::string& name );

	TiXmlAttribute* FirstAttribute() const	{ return attributeSet.First(); }		///< Access the first attribute in this element.
	TiXmlAttribute* LastAttribute()	const 	{ return attributeSet.Last(); }		///< Access the last attribute in this element.

	// [internal use] Creates a new Element and returs it.
	virtual TiXmlNode* Clone() const;
	// [internal use] 
 	virtual void Print( FILE* cfile, int depth ) const;
	// [internal use] 
	virtual void StreamOut ( std::ostream* out ) const;
	// [internal use] 
	virtual void StreamIn( std::istream* in, std::string* tag );

  protected:
	/*	[internal use] 
		Attribtue parsing starts: next char past '<'
						 returns: next char past '>'
	*/	
	virtual const char* Parse( const char* p );

	/*	[internal use]
		Reads the "value" of the element -- another element, or text.
		This should terminate with the current end tag.
	*/
	const char* ReadValue( const char* in );
	bool ReadValue( std::istream* in );

  private:
	TiXmlAttributeSet attributeSet;
};


/**	An XML comment.
*/
class TiXmlComment : public TiXmlNode
{
  public:
	/// Constructs an empty comment.
	TiXmlComment() : TiXmlNode( TiXmlNode::COMMENT ) {}
	virtual ~TiXmlComment()	{}

	// [internal use] Creates a new Element and returs it.
	virtual TiXmlNode* Clone() const;
	// [internal use] 
 	virtual void Print( FILE* cfile, int depth ) const;
	// [internal use] 
	virtual void StreamOut ( std::ostream* out ) const;
	// [internal use] 
	virtual void StreamIn( std::istream* in, std::string* tag );

  protected:
	/*	[internal use] 
		Attribtue parsing starts: at the ! of the !--
						 returns: next char past '>'
	*/	
	virtual const char* Parse( const char* p );
};


/** XML text. Contained in an element.
*/
class TiXmlText : public TiXmlNode
{
  public:
	TiXmlText( const std::string& initValue )  : TiXmlNode( TiXmlNode::TEXT ) { SetValue( initValue ); }
	virtual ~TiXmlText() {}


	// [internal use] Creates a new Element and returns it.
	virtual TiXmlNode* Clone() const;
	// [internal use] 
 	virtual void Print( FILE* cfile, int depth ) const;
	// [internal use] 
	virtual void StreamOut ( std::ostream* out ) const;
	// [internal use] 	
	bool Blank() const;	// returns true if all white space and new lines
	/*	[internal use] 
		Attribtue parsing starts: First char of the text
						 returns: next char past '>'
	*/	
	virtual const char* Parse( const char* p );
	// [internal use]
	virtual void StreamIn( std::istream* in, std::string* tag );
};


/** In correct XML the declaration is the first entry in the file.
	@verbatim
		<?xml version="1.0" standalone="yes"?>
	@endverbatim

	TinyXml will happily read or write files without a declaration,
	however. There are 3 possible attributes to the declaration: 
	version, encoding, and standalone.

	Note: In this version of the code, the attributes are
	handled as special cases, not generic attributes, simply
	because there can only be at most 3 and they are always the same.
*/
class TiXmlDeclaration : public TiXmlNode
{
  public:
	/// Construct an empty declaration.
	TiXmlDeclaration()   : TiXmlNode( TiXmlNode::DECLARATION ) {}

	/// Construct.
	TiXmlDeclaration( const std::string& version, 
					  const std::string& encoding,
					  const std::string& standalone );

	virtual ~TiXmlDeclaration()	{}

	/// Version. Will return empty if none was found.
	const std::string& Version() const		{ return version; }
	/// Encoding. Will return empty if none was found.
	const std::string& Encoding() const		{ return encoding; }
	/// Is this a standalone document? 
	const std::string& Standalone() const		{ return standalone; }

	// [internal use] Creates a new Element and returs it.
	virtual TiXmlNode* Clone() const;
	// [internal use] 
 	virtual void Print( FILE* cfile, int depth ) const;
	// [internal use] 
	virtual void StreamOut ( std::ostream* out ) const;
	// [internal use] 
	virtual void StreamIn( std::istream* in, std::string* tag );

  protected:
	//	[internal use] 
	//	Attribtue parsing starts: next char past '<'
	//					 returns: next char past '>'
	
	virtual const char* Parse( const char* p );

  private:
	std::string version;
	std::string encoding;
	std::string standalone;
};


/** Any tag that tinyXml doesn't recognize is save as an 
	unknown. It is a tag of text, but should not be modified.
	It will be written back to the XML, unchanged, when the file 
	is saved.
*/
class TiXmlUnknown : public TiXmlNode
{
  public:
	TiXmlUnknown() : TiXmlNode( TiXmlNode::UNKNOWN ) {}
	virtual ~TiXmlUnknown() {}

	// [internal use] 	
	virtual TiXmlNode* Clone() const;
	// [internal use] 
 	virtual void Print( FILE* cfile, int depth ) const;
	// [internal use] 
	virtual void StreamOut ( std::ostream* out ) const;
	// [internal use] 
	virtual void StreamIn( std::istream* in, std::string* tag );

  protected:
	/*	[internal use] 
		Attribute parsing starts: First char of the text
						 returns: next char past '>'
	*/	
	virtual const char* Parse( const char* p );
};


/** Always the top level node. A document binds together all the
	XML pieces. It can be saved, loaded, and printed to the screen.
	The 'value' of a document node is the xml file name.
*/
class TiXmlDocument : public TiXmlNode
{
  public:
	/// Create an empty document, that has no name.
	TiXmlDocument();
	/// Create a document with a name. The name of the document is also the filename of the xml.
	TiXmlDocument( const std::string& documentName );
	
	virtual ~TiXmlDocument() {}

	/** Load a file using the current document value. 
		Returns true if successful. Will delete any existing
		document data before loading.
	*/
	bool LoadFile();
	/// Save a file using the current document value. Returns true if successful.
	bool SaveFile() const;
	/// Load a file using the given filename. Returns true if successful.
	bool LoadFile( const std::string& filename );
	/// Save a file using the given filename. Returns true if successful.
	bool SaveFile( const std::string& filename ) const;

	/// Parse the given null terminated block of xml data.
	virtual const char* Parse( const char* p );

	/** Get the root element -- the only top level element -- of the document.
		In well formed XML, there should only be one. TinyXml is tolerant of
		multiple elements at the document level.
	*/
	TiXmlElement* RootElement() const		{ return FirstChildElement(); }
	
	/// If, during parsing, a error occurs, Error will be set to true.
	bool Error() const						{ return error; }

	/// Contains a textual (english) description of the error if one occurs.
	const std::string& ErrorDesc() const	{ return errorDesc; }

	/** Generally, you probably want the error string ( ErrorDesc() ). But if you
		prefer the ErrorId, this function will fetch it.
	*/
	const int ErrorId()	const				{ return errorId; }

	/// If you have handled the error, it can be reset with this call.
	void ClearError()						{ error = false; errorId = 0; errorDesc = ""; }
  
	/** Dump the document to standard out. */
	void Print() const								{ Print( stdout, 0 ); }

	// [internal use] 
 	virtual void Print( FILE* cfile, int depth = 0 ) const;
	// [internal use] 
	virtual void StreamOut ( std::ostream* out ) const;
	// [internal use] 	
	virtual TiXmlNode* Clone() const;
	// [internal use] 	
	void SetError( int err ) {		assert( err > 0 && err < TIXML_ERROR_STRING_COUNT );
									error   = true; 
									errorId = err;
									errorDesc = errorString[ errorId ]; }
	// [internal use] 
	virtual void StreamIn( std::istream* in, std::string* tag );

  protected:

  private:
	bool error;
	int  errorId;	
	std::string errorDesc;
};



#endif

