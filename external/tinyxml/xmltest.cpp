/*
   Test program for TinyXML.
*/


#ifdef TIXML_USE_STL
	#include <iostream>
	#include <sstream>
	using namespace std;
#else
	#include <stdio.h>
#endif

#if defined( WIN32 ) && defined( TUNE )
	#include <crtdbg.h>
	_CrtMemState startMemState;
	_CrtMemState endMemState;
#endif

#include "tinyxml.h"

bool XmlTest (const char* testString, const char* expected, const char* found, bool noEcho = false);
bool XmlTest( const char* testString, int expected, int found, bool noEcho = false );

static int gPass = 0;
static int gFail = 0;



bool XmlTest (const char* testString, const char* expected, const char* found, bool noEcho )
{
	bool pass = !strcmp( expected, found );
	if ( pass )
		printf ("[pass]");
	else
		printf ("[fail]");

	if ( noEcho )
		printf (" %s\n", testString);
	else
		printf (" %s [%s][%s]\n", testString, expected, found);

	if ( pass )
		++gPass;
	else
		++gFail;
	return pass;
}


bool XmlTest( const char* testString, int expected, int found, bool noEcho )
{
	bool pass = ( expected == found );
	if ( pass )
		printf ("[pass]");
	else
		printf ("[fail]");

	if ( noEcho )
		printf (" %s\n", testString);
	else
		printf (" %s [%d][%d]\n", testString, expected, found);

	if ( pass )
		++gPass;
	else
		++gFail;
	return pass;
}


void NullLineEndings( char* p )
{
	while( p && *p ) {
		if ( *p == '\n' || *p == '\r' ) {
			*p = 0;
			return;
		}
		++p;
	}
}

//
// This file demonstrates some basic functionality of TinyXml.
// Note that the example is very contrived. It presumes you know
// what is in the XML file. But it does test the basic operations,
// and show how to add and remove nodes.
//

int main()
{

	//
	// We start with the 'demoStart' todo list. Process it. And
	// should hopefully end up with the todo list as illustrated.
	//
	const char* demoStart =
		"<?xml version=\"1.0\"  standalone='no' >\n"
		"<!-- Our to do list data -->"
		"<ToDo>\n"
		"<!-- Do I need a secure PDA? -->\n"
		"<Item priority=\"1\" distance='close'> Go to the <bold>Toy store!</bold></Item>"
		"<Item priority=\"2\" distance='none'> Do bills   </Item>"
		"<Item priority=\"2\" distance='far &amp; back'> Look for Evil Dinosaurs! </Item>"
		"</ToDo>";
		
	{

	#ifdef TIXML_USE_STL
		//	What the todo list should look like after processing.
		// In stream (no formatting) representation.
		const char* demoEnd =
			"<?xml version=\"1.0\" standalone=\"no\" ?>"
			"<!-- Our to do list data -->"
			"<ToDo>"
			"<!-- Do I need a secure PDA? -->"
			"<Item priority=\"2\" distance=\"close\">Go to the"
			"<bold>Toy store!"
			"</bold>"
			"</Item>"
			"<Item priority=\"1\" distance=\"far\">Talk to:"
			"<Meeting where=\"School\">"
			"<Attendee name=\"Marple\" position=\"teacher\" />"
			"<Attendee name=\"Voel\" position=\"counselor\" />"
			"</Meeting>"
			"<Meeting where=\"Lunch\" />"
			"</Item>"
			"<Item priority=\"2\" distance=\"here\">Do bills"
			"</Item>"
			"</ToDo>";
	#endif

		// The example parses from the character string (above):
		#if defined( WIN32 ) && defined( TUNE )
		_CrtMemCheckpoint( &startMemState );
		#endif	

		{
			// Write to a file and read it back, to check file I/O.

			TiXmlDocument doc( "demotest.xml" );
			doc.Parse( demoStart );

			if ( doc.Error() )
			{
				printf( "Error in %s: %s\n", doc.Value(), doc.ErrorDesc() );
				exit( 1 );
			}
			doc.SaveFile();
		}

		TiXmlDocument doc( "demotest.xml" );
		bool loadOkay = doc.LoadFile();

		if ( !loadOkay )
		{
			printf( "Could not load test file 'demotest.xml'. Error='%s'. Exiting.\n", doc.ErrorDesc() );
			exit( 1 );
		}

		printf( "** Demo doc read from disk: ** \n\n" );
		printf( "** Printing via doc.Print **\n" );
		doc.Print( stdout );

		{
			printf( "** Printing via TiXmlPrinter **\n" );
			TiXmlPrinter printer;
			doc.Accept( &printer );
			fprintf( stdout, "%s", printer.CStr() );
		}
		#ifdef TIXML_USE_STL	
		{
			printf( "** Printing via operator<< **\n" );
			std::cout << doc;
		}
		#endif
		TiXmlNode* node = 0;
		TiXmlElement* todoElement = 0;
		TiXmlElement* itemElement = 0;


		// --------------------------------------------------------
		// An example of changing existing attributes, and removing
		// an element from the document.
		// --------------------------------------------------------

		// Get the "ToDo" element.
		// It is a child of the document, and can be selected by name.
		node = doc.FirstChild( "ToDo" );
		assert( node );
		todoElement = node->ToElement();
		assert( todoElement  );

		// Going to the toy store is now our second priority...
		// So set the "priority" attribute of the first item in the list.
		node = todoElement->FirstChildElement();	// This skips the "PDA" comment.
		assert( node );
		itemElement = node->ToElement();
		assert( itemElement  );
		itemElement->SetAttribute( "priority", 2 );

		// Change the distance to "doing bills" from
		// "none" to "here". It's the next sibling element.
		itemElement = itemElement->NextSiblingElement();
		assert( itemElement );
		itemElement->SetAttribute( "distance", "here" );

		// Remove the "Look for Evil Dinosaurs!" item.
		// It is 1 more sibling away. We ask the parent to remove
		// a particular child.
		itemElement = itemElement->NextSiblingElement();
		todoElement->RemoveChild( itemElement );

		itemElement = 0;

		// --------------------------------------------------------
		// What follows is an example of created elements and text
		// nodes and adding them to the document.
		// --------------------------------------------------------

		// Add some meetings.
		TiXmlElement item( "Item" );
		item.SetAttribute( "priority", "1" );
		item.SetAttribute( "distance", "far" );

		TiXmlText text( "Talk to:" );

		TiXmlElement meeting1( "Meeting" );
		meeting1.SetAttribute( "where", "School" );

		TiXmlElement meeting2( "Meeting" );
		meeting2.SetAttribute( "where", "Lunch" );

		TiXmlElement attendee1( "Attendee" );
		attendee1.SetAttribute( "name", "Marple" );
		attendee1.SetAttribute( "position", "teacher" );

		TiXmlElement attendee2( "Attendee" );
		attendee2.SetAttribute( "name", "Voel" );
		attendee2.SetAttribute( "position", "counselor" );

		// Assemble the nodes we've created:
		meeting1.InsertEndChild( attendee1 );
		meeting1.InsertEndChild( attendee2 );

		item.InsertEndChild( text );
		item.InsertEndChild( meeting1 );
		item.InsertEndChild( meeting2 );

		// And add the node to the existing list after the first child.
		node = todoElement->FirstChild( "Item" );
		assert( node );
		itemElement = node->ToElement();
		assert( itemElement );

		todoElement->InsertAfterChild( itemElement, item );

		printf( "\n** Demo doc processed: ** \n\n" );
		doc.Print( stdout );


	#ifdef TIXML_USE_STL
		printf( "** Demo doc processed to stream: ** \n\n" );
		cout << doc << endl << endl;
	#endif

		// --------------------------------------------------------
		// Different tests...do we have what we expect?
		// --------------------------------------------------------

		int count = 0;
		TiXmlElement*	element;

		//////////////////////////////////////////////////////

	#ifdef TIXML_USE_STL
		cout << "** Basic structure. **\n";
		ostringstream outputStream( ostringstream::out );
		outputStream << doc;
		XmlTest( "Output stream correct.",	string( demoEnd ).c_str(),
											outputStream.str().c_str(), true );
	#endif

		node = doc.RootElement();
		assert( node );
		XmlTest( "Root element exists.", true, ( node != 0 && node->ToElement() ) );
		XmlTest ( "Root element value is 'ToDo'.", "ToDo",  node->Value());

		node = node->FirstChild();
		XmlTest( "First child exists & is a comment.", true, ( node != 0 && node->ToComment() ) );
		node = node->NextSibling();
		XmlTest( "Sibling element exists & is an element.", true, ( node != 0 && node->ToElement() ) );
		XmlTest ( "Value is 'Item'.", "Item", node->Value() );

		node = node->FirstChild();
		XmlTest ( "First child exists.", true, ( node != 0 && node->ToText() ) );
		XmlTest ( "Value is 'Go to the'.", "Go to the", node->Value() );


		//////////////////////////////////////////////////////
		printf ("\n** Iterators. **\n");

		// Walk all the top level nodes of the document.
		count = 0;
		for( node = doc.FirstChild();
			 node;
			 node = node->NextSibling() )
		{
			count++;
		}
		XmlTest( "Top level nodes, using First / Next.", 3, count );

		count = 0;
		for( node = doc.LastChild();
			 node;
			 node = node->PreviousSibling() )
		{
			count++;
		}
		XmlTest( "Top level nodes, using Last / Previous.", 3, count );

		// Walk all the top level nodes of the document,
		// using a different syntax.
		count = 0;
		for( node = doc.IterateChildren( 0 );
			 node;
			 node = doc.IterateChildren( node ) )
		{
			count++;
		}
		XmlTest( "Top level nodes, using IterateChildren.", 3, count );

		// Walk all the elements in a node.
		count = 0;
		for( element = todoElement->FirstChildElement();
			 element;
			 element = element->NextSiblingElement() )
		{
			count++;
		}
		XmlTest( "Children of the 'ToDo' element, using First / Next.",
			3, count );

		// Walk all the elements in a node by value.
		count = 0;
		for( node = todoElement->FirstChild( "Item" );
			 node;
			 node = node->NextSibling( "Item" ) )
		{
			count++;
		}
		XmlTest( "'Item' children of the 'ToDo' element, using First/Next.", 3, count );

		count = 0;
		for( node = todoElement->LastChild( "Item" );
			 node;
			 node = node->PreviousSibling( "Item" ) )
		{
			count++;
		}
		XmlTest( "'Item' children of the 'ToDo' element, using Last/Previous.", 3, count );

	#ifdef TIXML_USE_STL
		{
			cout << "\n** Parsing. **\n";
			istringstream parse0( "<Element0 attribute0='foo0' attribute1= noquotes attribute2 = '&gt;' />" );
			TiXmlElement element0( "default" );
			parse0 >> element0;

			XmlTest ( "Element parsed, value is 'Element0'.", "Element0", element0.Value() );
			XmlTest ( "Reads attribute 'attribute0=\"foo0\"'.", "foo0", element0.Attribute( "attribute0" ));
			XmlTest ( "Reads incorrectly formatted 'attribute1=noquotes'.", "noquotes", element0.Attribute( "attribute1" ) );
			XmlTest ( "Read attribute with entity value '>'.", ">", element0.Attribute( "attribute2" ) );
		}
	#endif

		{
			const char* error =	"<?xml version=\"1.0\" standalone=\"no\" ?>\n"
								"<passages count=\"006\" formatversion=\"20020620\">\n"
								"    <wrong error>\n"
								"</passages>";

			TiXmlDocument docTest;
			docTest.Parse( error );
			XmlTest( "Error row", docTest.ErrorRow(), 3 );
			XmlTest( "Error column", docTest.ErrorCol(), 17 );
			//printf( "error=%d id='%s' row %d col%d\n", (int) doc.Error(), doc.ErrorDesc(), doc.ErrorRow()+1, doc.ErrorCol() + 1 );

		}

	#ifdef TIXML_USE_STL
		{
			//////////////////////////////////////////////////////
			cout << "\n** Streaming. **\n";

			// Round trip check: stream in, then stream back out to verify. The stream
			// out has already been checked, above. We use the output

			istringstream inputStringStream( outputStream.str() );
			TiXmlDocument document0;

			inputStringStream >> document0;

			ostringstream outputStream0( ostringstream::out );
			outputStream0 << document0;

			XmlTest( "Stream round trip correct.",	string( demoEnd ).c_str(), 
													outputStream0.str().c_str(), true );

			std::string str;
			str << document0;

			XmlTest( "String printing correct.", string( demoEnd ).c_str(), 
												 str.c_str(), true );
		}
	#endif
	}

	{
		const char* str = "<doc attr0='1' attr1='2.0' attr2='foo' />";

		TiXmlDocument doc;
		doc.Parse( str );

		TiXmlElement* ele = doc.FirstChildElement();

		int iVal, result;
		double dVal;

		result = ele->QueryDoubleAttribute( "attr0", &dVal );
		XmlTest( "Query attribute: int as double", result, TIXML_SUCCESS );
		XmlTest( "Query attribute: int as double", (int)dVal, 1 );
		result = ele->QueryDoubleAttribute( "attr1", &dVal );
		XmlTest( "Query attribute: double as double", (int)dVal, 2 );
		result = ele->QueryIntAttribute( "attr1", &iVal );
		XmlTest( "Query attribute: double as int", result, TIXML_SUCCESS );
		XmlTest( "Query attribute: double as int", iVal, 2 );
		result = ele->QueryIntAttribute( "attr2", &iVal );
		XmlTest( "Query attribute: not a number", result, TIXML_WRONG_TYPE );
		result = ele->QueryIntAttribute( "bar", &iVal );
		XmlTest( "Query attribute: does not exist", result, TIXML_NO_ATTRIBUTE );
	}

	{
		const char* str = "<doc/>";

		TiXmlDocument doc;
		doc.Parse( str );

		TiXmlElement* ele = doc.FirstChildElement();

		int iVal;
		double dVal;

		ele->SetAttribute( "str", "strValue" );
		ele->SetAttribute( "int", 1 );
		ele->SetDoubleAttribute( "double", -1.0 );

		const char* cStr = ele->Attribute( "str" );
		ele->QueryIntAttribute( "int", &iVal );
		ele->QueryDoubleAttribute( "double", &dVal );

		XmlTest( "Attribute round trip. c-string.", "strValue", cStr );
		XmlTest( "Attribute round trip. int.", 1, iVal );
		XmlTest( "Attribute round trip. double.", -1, (int)dVal );
	}
	
	{
		const char* str =	"\t<?xml version=\"1.0\" standalone=\"no\" ?>\t<room doors='2'>\n"
							"</room>";

		TiXmlDocument doc;
		doc.SetTabSize( 8 );
		doc.Parse( str );

		TiXmlHandle docHandle( &doc );
		TiXmlHandle roomHandle = docHandle.FirstChildElement( "room" );

		assert( docHandle.Node() );
		assert( roomHandle.Element() );

		TiXmlElement* room = roomHandle.Element();
		assert( room );
		TiXmlAttribute* doors = room->FirstAttribute();
		assert( doors );

		XmlTest( "Location tracking: Tab 8: room row", room->Row(), 1 );
		XmlTest( "Location tracking: Tab 8: room col", room->Column(), 49 );
		XmlTest( "Location tracking: Tab 8: doors row", doors->Row(), 1 );
		XmlTest( "Location tracking: Tab 8: doors col", doors->Column(), 55 );
	}
	
	{
		const char* str =	"\t<?xml version=\"1.0\" standalone=\"no\" ?>\t<room doors='2'>\n"
							"  <!-- Silly example -->\n"
							"    <door wall='north'>A great door!</door>\n"
							"\t<door wall='east'/>"
							"</room>";

		TiXmlDocument doc;
		doc.Parse( str );

		TiXmlHandle docHandle( &doc );
		TiXmlHandle roomHandle = docHandle.FirstChildElement( "room" );
		TiXmlHandle commentHandle = docHandle.FirstChildElement( "room" ).FirstChild();
		TiXmlHandle textHandle = docHandle.FirstChildElement( "room" ).ChildElement( "door", 0 ).FirstChild();
		TiXmlHandle door0Handle = docHandle.FirstChildElement( "room" ).ChildElement( 0 );
		TiXmlHandle door1Handle = docHandle.FirstChildElement( "room" ).ChildElement( 1 );

		assert( docHandle.Node() );
		assert( roomHandle.Element() );
		assert( commentHandle.Node() );
		assert( textHandle.Text() );
		assert( door0Handle.Element() );
		assert( door1Handle.Element() );

		TiXmlDeclaration* declaration = doc.FirstChild()->ToDeclaration();
		assert( declaration );
		TiXmlElement* room = roomHandle.Element();
		assert( room );
		TiXmlAttribute* doors = room->FirstAttribute();
		assert( doors );
		TiXmlText* text = textHandle.Text();
		TiXmlComment* comment = commentHandle.Node()->ToComment();
		assert( comment );
		TiXmlElement* door0 = door0Handle.Element();
		TiXmlElement* door1 = door1Handle.Element();

		XmlTest( "Location tracking: Declaration row", declaration->Row(), 1 );
		XmlTest( "Location tracking: Declaration col", declaration->Column(), 5 );
		XmlTest( "Location tracking: room row", room->Row(), 1 );
		XmlTest( "Location tracking: room col", room->Column(), 45 );
		XmlTest( "Location tracking: doors row", doors->Row(), 1 );
		XmlTest( "Location tracking: doors col", doors->Column(), 51 );
		XmlTest( "Location tracking: Comment row", comment->Row(), 2 );
		XmlTest( "Location tracking: Comment col", comment->Column(), 3 );
		XmlTest( "Location tracking: text row", text->Row(), 3 ); 
		XmlTest( "Location tracking: text col", text->Column(), 24 );
		XmlTest( "Location tracking: door0 row", door0->Row(), 3 );
		XmlTest( "Location tracking: door0 col", door0->Column(), 5 );
		XmlTest( "Location tracking: door1 row", door1->Row(), 4 );
		XmlTest( "Location tracking: door1 col", door1->Column(), 5 );
	}


	// --------------------------------------------------------
	// UTF-8 testing. It is important to test:
	//	1. Making sure name, value, and text read correctly
	//	2. Row, Col functionality
	//	3. Correct output
	// --------------------------------------------------------
	printf ("\n** UTF-8 **\n");
	{
		TiXmlDocument doc( "utf8test.xml" );
		doc.LoadFile();
		if ( doc.Error() && doc.ErrorId() == TiXmlBase::TIXML_ERROR_OPENING_FILE ) {
			printf( "WARNING: File 'utf8test.xml' not found.\n"
					"(Are you running the test from the wrong directory?)\n"
				    "Could not test UTF-8 functionality.\n" );
		}
		else
		{
			TiXmlHandle docH( &doc );
			// Get the attribute "value" from the "Russian" element and check it.
			TiXmlElement* element = docH.FirstChildElement( "document" ).FirstChildElement( "Russian" ).Element();
			const unsigned char correctValue[] = {	0xd1U, 0x86U, 0xd0U, 0xb5U, 0xd0U, 0xbdU, 0xd0U, 0xbdU, 
													0xd0U, 0xbeU, 0xd1U, 0x81U, 0xd1U, 0x82U, 0xd1U, 0x8cU, 0 };

			XmlTest( "UTF-8: Russian value.", (const char*)correctValue, element->Attribute( "value" ), true );
			XmlTest( "UTF-8: Russian value row.", 4, element->Row() );
			XmlTest( "UTF-8: Russian value column.", 5, element->Column() );

			const unsigned char russianElementName[] = {	0xd0U, 0xa0U, 0xd1U, 0x83U,
															0xd1U, 0x81U, 0xd1U, 0x81U,
															0xd0U, 0xbaU, 0xd0U, 0xb8U,
															0xd0U, 0xb9U, 0 };
			const char russianText[] = "<\xD0\xB8\xD0\xBC\xD0\xB5\xD0\xB5\xD1\x82>";

			TiXmlText* text = docH.FirstChildElement( "document" ).FirstChildElement( (const char*) russianElementName ).Child( 0 ).Text();
			XmlTest( "UTF-8: Browsing russian element name.",
					 russianText,
					 text->Value(),
					 true );
			XmlTest( "UTF-8: Russian element name row.", 7, text->Row() );
			XmlTest( "UTF-8: Russian element name column.", 47, text->Column() );

			TiXmlDeclaration* dec = docH.Child( 0 ).Node()->ToDeclaration();
			XmlTest( "UTF-8: Declaration column.", 1, dec->Column() );
			XmlTest( "UTF-8: Document column.", 1, doc.Column() );

			// Now try for a round trip.
			doc.SaveFile( "utf8testout.xml" );

			// Check the round trip.
			char savedBuf[256];
			char verifyBuf[256];
			int okay = 1;

			FILE* saved  = fopen( "utf8testout.xml", "r" );
			FILE* verify = fopen( "utf8testverify.xml", "r" );

			//bool firstLineBOM=true;
			if ( saved && verify )
			{
				while ( fgets( verifyBuf, 256, verify ) )
				{
					fgets( savedBuf, 256, saved );
					NullLineEndings( verifyBuf );
					NullLineEndings( savedBuf );

					if ( /*!firstLineBOM && */ strcmp( verifyBuf, savedBuf ) )
					{
						printf( "verify:%s<\n", verifyBuf );
						printf( "saved :%s<\n", savedBuf );
						okay = 0;
						break;
					}
					//firstLineBOM = false;
				}
			}
			if ( saved )
				fclose( saved );
			if ( verify )
				fclose( verify );
			XmlTest( "UTF-8: Verified multi-language round trip.", 1, okay );

			// On most Western machines, this is an element that contains
			// the word "resume" with the correct accents, in a latin encoding.
			// It will be something else completely on non-wester machines,
			// which is why TinyXml is switching to UTF-8.
			const char latin[] = "<element>r\x82sum\x82</element>";

			TiXmlDocument latinDoc;
			latinDoc.Parse( latin, 0, TIXML_ENCODING_LEGACY );

			text = latinDoc.FirstChildElement()->FirstChild()->ToText();
			XmlTest( "Legacy encoding: Verify text element.", "r\x82sum\x82", text->Value() );
		}
	}		

	//////////////////////
	// Copy and assignment
	//////////////////////
	printf ("\n** Copy and Assignment **\n");
	{
		TiXmlElement element( "foo" );
		element.Parse( "<element name='value' />", 0, TIXML_ENCODING_UNKNOWN );

		TiXmlElement elementCopy( element );
		TiXmlElement elementAssign( "foo" );
		elementAssign.Parse( "<incorrect foo='bar'/>", 0, TIXML_ENCODING_UNKNOWN );
		elementAssign = element;

		XmlTest( "Copy/Assign: element copy #1.", "element", elementCopy.Value() );
		XmlTest( "Copy/Assign: element copy #2.", "value", elementCopy.Attribute( "name" ) );
		XmlTest( "Copy/Assign: element assign #1.", "element", elementAssign.Value() );
		XmlTest( "Copy/Assign: element assign #2.", "value", elementAssign.Attribute( "name" ) );
		XmlTest( "Copy/Assign: element assign #3.", true, ( 0 == elementAssign.Attribute( "foo" )) );

		TiXmlComment comment;
		comment.Parse( "<!--comment-->", 0, TIXML_ENCODING_UNKNOWN );
		TiXmlComment commentCopy( comment );
		TiXmlComment commentAssign;
		commentAssign = commentCopy;
		XmlTest( "Copy/Assign: comment copy.", "comment", commentCopy.Value() );
		XmlTest( "Copy/Assign: comment assign.", "comment", commentAssign.Value() );

		TiXmlUnknown unknown;
		unknown.Parse( "<[unknown]>", 0, TIXML_ENCODING_UNKNOWN );
		TiXmlUnknown unknownCopy( unknown );
		TiXmlUnknown unknownAssign;
		unknownAssign.Parse( "incorrect", 0, TIXML_ENCODING_UNKNOWN );
		unknownAssign = unknownCopy;
		XmlTest( "Copy/Assign: unknown copy.", "[unknown]", unknownCopy.Value() );
		XmlTest( "Copy/Assign: unknown assign.", "[unknown]", unknownAssign.Value() );
		
		TiXmlText text( "TextNode" );
		TiXmlText textCopy( text );
		TiXmlText textAssign( "incorrect" );
		textAssign = text;
		XmlTest( "Copy/Assign: text copy.", "TextNode", textCopy.Value() );
		XmlTest( "Copy/Assign: text assign.", "TextNode", textAssign.Value() );

		TiXmlDeclaration dec;
		dec.Parse( "<?xml version='1.0' encoding='UTF-8'?>", 0, TIXML_ENCODING_UNKNOWN );
		TiXmlDeclaration decCopy( dec );
		TiXmlDeclaration decAssign;
		decAssign = dec;

		XmlTest( "Copy/Assign: declaration copy.", "UTF-8", decCopy.Encoding() );
		XmlTest( "Copy/Assign: text assign.", "UTF-8", decAssign.Encoding() );

		TiXmlDocument doc;
		elementCopy.InsertEndChild( textCopy );
		doc.InsertEndChild( decAssign );
		doc.InsertEndChild( elementCopy );
		doc.InsertEndChild( unknownAssign );

		TiXmlDocument docCopy( doc );
		TiXmlDocument docAssign;
		docAssign = docCopy;

		#ifdef TIXML_USE_STL
		std::string original, copy, assign;
		original << doc;
		copy << docCopy;
		assign << docAssign;
		XmlTest( "Copy/Assign: document copy.", original.c_str(), copy.c_str(), true );
		XmlTest( "Copy/Assign: document assign.", original.c_str(), assign.c_str(), true );

		#endif
	}	

	//////////////////////////////////////////////////////
#ifdef TIXML_USE_STL
	printf ("\n** Parsing, no Condense Whitespace **\n");
	TiXmlBase::SetCondenseWhiteSpace( false );
	{
		istringstream parse1( "<start>This  is    \ntext</start>" );
		TiXmlElement text1( "text" );
		parse1 >> text1;

		XmlTest ( "Condense white space OFF.", "This  is    \ntext",
					text1.FirstChild()->Value(),
					true );
	}
	TiXmlBase::SetCondenseWhiteSpace( true );
#endif

	//////////////////////////////////////////////////////
	// GetText();
	{
		const char* str = "<foo>This is text</foo>";
		TiXmlDocument doc;
		doc.Parse( str );
		const TiXmlElement* element = doc.RootElement();

		XmlTest( "GetText() normal use.", "This is text", element->GetText() );

		str = "<foo><b>This is text</b></foo>";
		doc.Clear();
		doc.Parse( str );
		element = doc.RootElement();

		XmlTest( "GetText() contained element.", element->GetText() == 0, true );

		str = "<foo>This is <b>text</b></foo>";
		doc.Clear();
		TiXmlBase::SetCondenseWhiteSpace( false );
		doc.Parse( str );
		TiXmlBase::SetCondenseWhiteSpace( true );
		element = doc.RootElement();

		XmlTest( "GetText() partial.", "This is ", element->GetText() );
	}


	//////////////////////////////////////////////////////
	// CDATA
	{
		const char* str =	"<xmlElement>"
								"<![CDATA["
									"I am > the rules!\n"
									"...since I make symbolic puns"
								"]]>"
							"</xmlElement>";
		TiXmlDocument doc;
		doc.Parse( str );
		doc.Print();

		XmlTest( "CDATA parse.", doc.FirstChildElement()->FirstChild()->Value(), 
								 "I am > the rules!\n...since I make symbolic puns",
								 true );

		#ifdef TIXML_USE_STL
		//cout << doc << '\n';

		doc.Clear();

		istringstream parse0( str );
		parse0 >> doc;
		//cout << doc << '\n';

		XmlTest( "CDATA stream.", doc.FirstChildElement()->FirstChild()->Value(), 
								 "I am > the rules!\n...since I make symbolic puns",
								 true );
		#endif

		TiXmlDocument doc1 = doc;
		//doc.Print();

		XmlTest( "CDATA copy.", doc1.FirstChildElement()->FirstChild()->Value(), 
								 "I am > the rules!\n...since I make symbolic puns",
								 true );
	}
	{
		// [ 1482728 ] Wrong wide char parsing
		char buf[256];
		buf[255] = 0;
		for( int i=0; i<255; ++i ) {
			buf[i] = (char)((i>=32) ? i : 32);
		}
		TIXML_STRING str( "<xmlElement><![CDATA[" );
		str += buf;
		str += "]]></xmlElement>";

		TiXmlDocument doc;
		doc.Parse( str.c_str() );

		TiXmlPrinter printer;
		printer.SetStreamPrinting();
		doc.Accept( &printer );

		XmlTest( "CDATA with all bytes #1.", str.c_str(), printer.CStr(), true );

		#ifdef TIXML_USE_STL
		doc.Clear();
		istringstream iss( printer.Str() );
		iss >> doc;
		std::string out;
		out << doc;
		XmlTest( "CDATA with all bytes #2.", out.c_str(), printer.CStr(), true );
		#endif
	}
	{
		// [ 1480107 ] Bug-fix for STL-streaming of CDATA that contains tags
		// CDATA streaming had a couple of bugs, that this tests for.
		const char* str =	"<xmlElement>"
								"<![CDATA["
									"<b>I am > the rules!</b>\n"
									"...since I make symbolic puns"
								"]]>"
							"</xmlElement>";
		TiXmlDocument doc;
		doc.Parse( str );
		doc.Print();

		XmlTest( "CDATA parse. [ 1480107 ]", doc.FirstChildElement()->FirstChild()->Value(), 
								 "<b>I am > the rules!</b>\n...since I make symbolic puns",
								 true );

		#ifdef TIXML_USE_STL

		doc.Clear();

		istringstream parse0( str );
		parse0 >> doc;

		XmlTest( "CDATA stream. [ 1480107 ]", doc.FirstChildElement()->FirstChild()->Value(), 
								 "<b>I am > the rules!</b>\n...since I make symbolic puns",
								 true );
		#endif

		TiXmlDocument doc1 = doc;
		//doc.Print();

		XmlTest( "CDATA copy. [ 1480107 ]", doc1.FirstChildElement()->FirstChild()->Value(), 
								 "<b>I am > the rules!</b>\n...since I make symbolic puns",
								 true );
	}
	//////////////////////////////////////////////////////
	// Visit()



	//////////////////////////////////////////////////////
	printf( "\n** Fuzzing... **\n" );

	const int FUZZ_ITERATION = 300;

	// The only goal is not to crash on bad input.
	int len = (int) strlen( demoStart );
	for( int i=0; i<FUZZ_ITERATION; ++i ) 
	{
		char* demoCopy = new char[ len+1 ];
		strcpy( demoCopy, demoStart );

		demoCopy[ i%len ] = (char)((i+1)*3);
		demoCopy[ (i*7)%len ] = '>';
		demoCopy[ (i*11)%len ] = '<';

		TiXmlDocument xml;
		xml.Parse( demoCopy );

		delete [] demoCopy;
	}
	printf( "** Fuzzing Complete. **\n" );
	
	//////////////////////////////////////////////////////
	printf ("\n** Bug regression tests **\n");

	// InsertBeforeChild and InsertAfterChild causes crash.
	{
		TiXmlElement parent( "Parent" );
		TiXmlElement childText0( "childText0" );
		TiXmlElement childText1( "childText1" );
		TiXmlNode* childNode0 = parent.InsertEndChild( childText0 );
		TiXmlNode* childNode1 = parent.InsertBeforeChild( childNode0, childText1 );

		XmlTest( "Test InsertBeforeChild on empty node.", ( childNode1 == parent.FirstChild() ), true );
	}

	{
		// InsertBeforeChild and InsertAfterChild causes crash.
		TiXmlElement parent( "Parent" );
		TiXmlElement childText0( "childText0" );
		TiXmlElement childText1( "childText1" );
		TiXmlNode* childNode0 = parent.InsertEndChild( childText0 );
		TiXmlNode* childNode1 = parent.InsertAfterChild( childNode0, childText1 );

		XmlTest( "Test InsertAfterChild on empty node. ", ( childNode1 == parent.LastChild() ), true );
	}

	// Reports of missing constructors, irregular string problems.
	{
		// Missing constructor implementation. No test -- just compiles.
		TiXmlText text( "Missing" );

		#ifdef TIXML_USE_STL
			// Missing implementation:
			TiXmlDocument doc;
			string name = "missing";
			doc.LoadFile( name );

			TiXmlText textSTL( name );
		#else
			// verifying some basic string functions:
			TiXmlString a;
			TiXmlString b( "Hello" );
			TiXmlString c( "ooga" );

			c = " World!";
			a = b;
			a += c;
			a = a;

			XmlTest( "Basic TiXmlString test. ", "Hello World!", a.c_str() );
		#endif
 	}

	// Long filenames crashing STL version
	{
		TiXmlDocument doc( "midsummerNightsDreamWithAVeryLongFilenameToConfuseTheStringHandlingRoutines.xml" );
		bool loadOkay = doc.LoadFile();
		loadOkay = true;	// get rid of compiler warning.
		// Won't pass on non-dev systems. Just a "no crash" check.
		//XmlTest( "Long filename. ", true, loadOkay );
	}

	{
		// Entities not being written correctly.
		// From Lynn Allen

		const char* passages =
			"<?xml version=\"1.0\" standalone=\"no\" ?>"
			"<passages count=\"006\" formatversion=\"20020620\">"
				"<psg context=\"Line 5 has &quot;quotation marks&quot; and &apos;apostrophe marks&apos;."
				" It also has &lt;, &gt;, and &amp;, as well as a fake copyright &#xA9;.\"> </psg>"
			"</passages>";

		TiXmlDocument doc( "passages.xml" );
		doc.Parse( passages );
		TiXmlElement* psg = doc.RootElement()->FirstChildElement();
		const char* context = psg->Attribute( "context" );
		const char* expected = "Line 5 has \"quotation marks\" and 'apostrophe marks'. It also has <, >, and &, as well as a fake copyright \xC2\xA9.";

		XmlTest( "Entity transformation: read. ", expected, context, true );

		FILE* textfile = fopen( "textfile.txt", "w" );
		if ( textfile )
		{
			psg->Print( textfile, 0 );
			fclose( textfile );
		}
		textfile = fopen( "textfile.txt", "r" );
		assert( textfile );
		if ( textfile )
		{
			char buf[ 1024 ];
			fgets( buf, 1024, textfile );
			XmlTest( "Entity transformation: write. ",
					 "<psg context=\'Line 5 has &quot;quotation marks&quot; and &apos;apostrophe marks&apos;."
					 " It also has &lt;, &gt;, and &amp;, as well as a fake copyright \xC2\xA9.' />",
					 buf,
					 true );
		}
		fclose( textfile );
	}

    {
		FILE* textfile = fopen( "test5.xml", "w" );
		if ( textfile )
		{
            fputs("<?xml version='1.0'?><a.elem xmi.version='2.0'/>", textfile);
            fclose(textfile);

			TiXmlDocument doc;
            doc.LoadFile( "test5.xml" );
            XmlTest( "dot in element attributes and names", doc.Error(), 0);
		}
    }

	{
		FILE* textfile = fopen( "test6.xml", "w" );
		if ( textfile )
		{
            fputs("<element><Name>1.1 Start easy ignore fin thickness&#xA;</Name></element>", textfile );
            fclose(textfile);

            TiXmlDocument doc;
            bool result = doc.LoadFile( "test6.xml" );
            XmlTest( "Entity with one digit.", result, true );

			TiXmlText* text = doc.FirstChildElement()->FirstChildElement()->FirstChild()->ToText();
			XmlTest( "Entity with one digit.",
						text->Value(), "1.1 Start easy ignore fin thickness\n" );
		}
    }

	{
		// DOCTYPE not preserved (950171)
		// 
		const char* doctype =
			"<?xml version=\"1.0\" ?>"
			"<!DOCTYPE PLAY SYSTEM 'play.dtd'>"
			"<!ELEMENT title (#PCDATA)>"
			"<!ELEMENT books (title,authors)>"
			"<element />";

		TiXmlDocument doc;
		doc.Parse( doctype );
		doc.SaveFile( "test7.xml" );
		doc.Clear();
		doc.LoadFile( "test7.xml" );
		
		TiXmlHandle docH( &doc );
		TiXmlUnknown* unknown = docH.Child( 1 ).Unknown();
		XmlTest( "Correct value of unknown.", "!DOCTYPE PLAY SYSTEM 'play.dtd'", unknown->Value() );
		#ifdef TIXML_USE_STL
		TiXmlNode* node = docH.Child( 2 ).Node();
		std::string str;
		str << (*node);
		XmlTest( "Correct streaming of unknown.", "<!ELEMENT title (#PCDATA)>", str.c_str() );
		#endif
	}

	{
		// [ 791411 ] Formatting bug
		// Comments do not stream out correctly.
		const char* doctype = 
			"<!-- Somewhat<evil> -->";
		TiXmlDocument doc;
		doc.Parse( doctype );

		TiXmlHandle docH( &doc );
		TiXmlComment* comment = docH.Child( 0 ).Node()->ToComment();

		XmlTest( "Comment formatting.", " Somewhat<evil> ", comment->Value() );
		#ifdef TIXML_USE_STL
		std::string str;
		str << (*comment);
		XmlTest( "Comment streaming.", "<!-- Somewhat<evil> -->", str.c_str() );
		#endif
	}

	{
		// [ 870502 ] White space issues
		TiXmlDocument doc;
		TiXmlText* text;
		TiXmlHandle docH( &doc );
	
		const char* doctype0 = "<element> This has leading and trailing space </element>";
		const char* doctype1 = "<element>This has  internal space</element>";
		const char* doctype2 = "<element> This has leading, trailing, and  internal space </element>";

		TiXmlBase::SetCondenseWhiteSpace( false );
		doc.Clear();
		doc.Parse( doctype0 );
		text = docH.FirstChildElement( "element" ).Child( 0 ).Text();
		XmlTest( "White space kept.", " This has leading and trailing space ", text->Value() );

		doc.Clear();
		doc.Parse( doctype1 );
		text = docH.FirstChildElement( "element" ).Child( 0 ).Text();
		XmlTest( "White space kept.", "This has  internal space", text->Value() );

		doc.Clear();
		doc.Parse( doctype2 );
		text = docH.FirstChildElement( "element" ).Child( 0 ).Text();
		XmlTest( "White space kept.", " This has leading, trailing, and  internal space ", text->Value() );

		TiXmlBase::SetCondenseWhiteSpace( true );
		doc.Clear();
		doc.Parse( doctype0 );
		text = docH.FirstChildElement( "element" ).Child( 0 ).Text();
		XmlTest( "White space condensed.", "This has leading and trailing space", text->Value() );

		doc.Clear();
		doc.Parse( doctype1 );
		text = docH.FirstChildElement( "element" ).Child( 0 ).Text();
		XmlTest( "White space condensed.", "This has internal space", text->Value() );

		doc.Clear();
		doc.Parse( doctype2 );
		text = docH.FirstChildElement( "element" ).Child( 0 ).Text();
		XmlTest( "White space condensed.", "This has leading, trailing, and internal space", text->Value() );
	}

	{
		// Double attributes
		const char* doctype = "<element attr='red' attr='blue' />";

		TiXmlDocument doc;
		doc.Parse( doctype );
		
		XmlTest( "Parsing repeated attributes.", true, doc.Error() );	// is an  error to tinyxml (didn't use to be, but caused issues)
		//XmlTest( "Parsing repeated attributes.", "blue", doc.FirstChildElement( "element" )->Attribute( "attr" ) );
	}

	{
		// Embedded null in stream.
		const char* doctype = "<element att\0r='red' attr='blue' />";

		TiXmlDocument doc;
		doc.Parse( doctype );
		XmlTest( "Embedded null throws error.", true, doc.Error() );

		#ifdef TIXML_USE_STL
		istringstream strm( doctype );
		doc.Clear();
		doc.ClearError();
		strm >> doc;
		XmlTest( "Embedded null throws error.", true, doc.Error() );
		#endif
	}

    {
            // Legacy mode test. (This test may only pass on a western system)
            const char* str =
                        "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>"
                        "<ä>"
                        "CöntäntßäöüÄÖÜ"
                        "</ä>";

            TiXmlDocument doc;
            doc.Parse( str );

            TiXmlHandle docHandle( &doc );
            TiXmlHandle aHandle = docHandle.FirstChildElement( "ä" );
            TiXmlHandle tHandle = aHandle.Child( 0 );
            assert( aHandle.Element() );
            assert( tHandle.Text() );
            XmlTest( "ISO-8859-1 Parsing.", "CöntäntßäöüÄÖÜ", tHandle.Text()->Value() );
    }

	{
		// Empty documents should return TIXML_ERROR_PARSING_EMPTY, bug 1070717
		const char* str = "    ";
		TiXmlDocument doc;
		doc.Parse( str );
		XmlTest( "Empty document error TIXML_ERROR_DOCUMENT_EMPTY", TiXmlBase::TIXML_ERROR_DOCUMENT_EMPTY, doc.ErrorId() );
	}
	#ifndef TIXML_USE_STL
	{
		// String equality. [ 1006409 ] string operator==/!= no worky in all cases
		TiXmlString temp;
		XmlTest( "Empty tinyxml string compare equal", ( temp == "" ), true );

		TiXmlString    foo;
		TiXmlString    bar( "" );
		XmlTest( "Empty tinyxml string compare equal", ( foo == bar ), true );
	}

	#endif
	{
		// Bug [ 1195696 ] from marlonism
		TiXmlBase::SetCondenseWhiteSpace(false); 
		TiXmlDocument xml; 
		xml.Parse("<text><break/>This hangs</text>"); 
		XmlTest( "Test safe error return.", xml.Error(), false );
	}

	{
		// Bug [ 1243992 ] - another infinite loop
		TiXmlDocument doc;
		doc.SetCondenseWhiteSpace(false);
		doc.Parse("<p><pb></pb>test</p>");
	} 
	{
		// Low entities
		TiXmlDocument xml;
		xml.Parse( "<test>&#x0e;</test>" );
		const char result[] = { 0x0e, 0 };
		XmlTest( "Low entities.", xml.FirstChildElement()->GetText(), result );
		xml.Print();
	}
	{
		// Bug [ 1451649 ] Attribute values with trailing quotes not handled correctly
		TiXmlDocument xml;
		xml.Parse( "<foo attribute=bar\" />" );
		XmlTest( "Throw error with bad end quotes.", xml.Error(), true );
	}
	#ifdef TIXML_USE_STL
	{
		// Bug [ 1449463 ] Consider generic query
		TiXmlDocument xml;
		xml.Parse( "<foo bar='3' barStr='a string'/>" );

		TiXmlElement* ele = xml.FirstChildElement();
		double d;
		int i;
		float f;
		bool b;
		std::string str;

		XmlTest( "QueryValueAttribute", ele->QueryValueAttribute( "bar", &d ), TIXML_SUCCESS );
		XmlTest( "QueryValueAttribute", ele->QueryValueAttribute( "bar", &i ), TIXML_SUCCESS );
		XmlTest( "QueryValueAttribute", ele->QueryValueAttribute( "bar", &f ), TIXML_SUCCESS );
		XmlTest( "QueryValueAttribute", ele->QueryValueAttribute( "bar", &b ), TIXML_WRONG_TYPE );
		XmlTest( "QueryValueAttribute", ele->QueryValueAttribute( "nobar", &b ), TIXML_NO_ATTRIBUTE );
		XmlTest( "QueryValueAttribute", ele->QueryValueAttribute( "barStr", &str ), TIXML_SUCCESS );

		XmlTest( "QueryValueAttribute", (d==3.0), true );
		XmlTest( "QueryValueAttribute", (i==3), true );
		XmlTest( "QueryValueAttribute", (f==3.0f), true );
		XmlTest( "QueryValueAttribute", (str==std::string( "a string" )), true );
	}
	#endif

	#ifdef TIXML_USE_STL
	{
		// [ 1505267 ] redundant malloc in TiXmlElement::Attribute
		TiXmlDocument xml;
		xml.Parse( "<foo bar='3' />" );
		TiXmlElement* ele = xml.FirstChildElement();
		double d;
		int i;

		std::string bar = "bar";

		const std::string* atrrib = ele->Attribute( bar );
		ele->Attribute( bar, &d );
		ele->Attribute( bar, &i );

		XmlTest( "Attribute", atrrib->empty(), false );
		XmlTest( "Attribute", (d==3.0), true );
		XmlTest( "Attribute", (i==3), true );
	}
	#endif

	{
		// [ 1356059 ] Allow TiXMLDocument to only be at the top level
		TiXmlDocument xml, xml2;
		xml.InsertEndChild( xml2 );
		XmlTest( "Document only at top level.", xml.Error(), true );
		XmlTest( "Document only at top level.", xml.ErrorId(), TiXmlBase::TIXML_ERROR_DOCUMENT_TOP_ONLY );
	}

	{
		// [ 1663758 ] Failure to report error on bad XML
		TiXmlDocument xml;
		xml.Parse("<x>");
		XmlTest("Missing end tag at end of input", xml.Error(), true);
		xml.Parse("<x> ");
		XmlTest("Missing end tag with trailing whitespace", xml.Error(), true);
	} 

	{
		// [ 1635701 ] fail to parse files with a tag separated into two lines
		// I'm not sure this is a bug. Marked 'pending' for feedback.
		TiXmlDocument xml;
		xml.Parse( "<title><p>text</p\n><title>" );
		//xml.Print();
		//XmlTest( "Tag split by newline", xml.Error(), false );
	}

	#ifdef TIXML_USE_STL
	{
		// [ 1475201 ] TinyXML parses entities in comments
		TiXmlDocument xml;
		istringstream parse1( "<!-- declarations for <head> & <body> -->"
						      "<!-- far &amp; away -->" );
		parse1 >> xml;

		TiXmlNode* e0 = xml.FirstChild();
		TiXmlNode* e1 = e0->NextSibling();
		TiXmlComment* c0 = e0->ToComment();
		TiXmlComment* c1 = e1->ToComment();

		XmlTest( "Comments ignore entities.", " declarations for <head> & <body> ", c0->Value(), true );
		XmlTest( "Comments ignore entities.", " far &amp; away ", c1->Value(), true );
	}
	#endif

	{
		// [ 1475201 ] TinyXML parses entities in comments
		TiXmlDocument xml;
		xml.Parse("<!-- declarations for <head> & <body> -->"
				  "<!-- far &amp; away -->" );

		TiXmlNode* e0 = xml.FirstChild();
		TiXmlNode* e1 = e0->NextSibling();
		TiXmlComment* c0 = e0->ToComment();
		TiXmlComment* c1 = e1->ToComment();

		XmlTest( "Comments ignore entities.", " declarations for <head> & <body> ", c0->Value(), true );
		XmlTest( "Comments ignore entities.", " far &amp; away ", c1->Value(), true );
	}

	{
		TiXmlDocument xml;
		xml.Parse( "<Parent>"
						"<child1 att=''/>"
						"<!-- With this comment, child2 will not be parsed! -->"
						"<child2 att=''/>"
					"</Parent>" );
		int count = 0;

		TiXmlNode* ele = 0;
		while ( (ele = xml.FirstChildElement( "Parent" )->IterateChildren( ele ) ) != 0 ) {
			++count;
		}
		XmlTest( "Comments iterate correctly.", 3, count );
	}

	{
		// trying to repro ]1874301]. If it doesn't go into an infinite loop, all is well.
		unsigned char buf[] = "<?xml version=\"1.0\" encoding=\"utf-8\"?><feed><![CDATA[Test XMLblablablalblbl";
		buf[60] = 239;
		buf[61] = 0;

		TiXmlDocument doc;
		doc.Parse( (const char*)buf);
	} 


	{
		// bug 1827248 Error while parsing a little bit malformed file
		// Actually not malformed - should work.
		TiXmlDocument xml;
		xml.Parse( "<attributelist> </attributelist >" );
		XmlTest( "Handle end tag whitespace", false, xml.Error() );
	}

	{
		// This one must not result in an infinite loop
		TiXmlDocument xml;
		xml.Parse( "<infinite>loop" );
		XmlTest( "Infinite loop test.", true, true );
	}

	{
		// 1709904 - can not repro the crash
		{
			TiXmlDocument xml;
			xml.Parse( "<tag>/</tag>" );
			XmlTest( "Odd XML parsing.", xml.FirstChild()->Value(), "tag" );
		}
		/* Could not repro. {
			TiXmlDocument xml;
			xml.LoadFile( "EQUI_Inventory.xml" );
			//XmlTest( "Odd XML parsing.", xml.FirstChildElement()->Value(), "XML" );
			TiXmlPrinter printer;
			xml.Accept( &printer );
			fprintf( stdout, "%s", printer.CStr() );
		}*/
	}

	/*  1417717 experiment
	{
		TiXmlDocument xml;
		xml.Parse("<text>Dan & Tracie</text>");
		xml.Print(stdout);
	}
	{
		TiXmlDocument xml;
		xml.Parse("<text>Dan &foo; Tracie</text>");
		xml.Print(stdout);
	}
	*/

	#if defined( WIN32 ) && defined( TUNE )
	_CrtMemCheckpoint( &endMemState );
	//_CrtMemDumpStatistics( &endMemState );

	_CrtMemState diffMemState;
	_CrtMemDifference( &diffMemState, &startMemState, &endMemState );
	_CrtMemDumpStatistics( &diffMemState );
	#endif

	printf ("\nPass %d, Fail %d\n", gPass, gFail);
	return gFail;
}
