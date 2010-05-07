#include "tinyxml.h"
#include <iostream>
#include <sstream>
#include <strstream>
using namespace std;

int gPass = 0;
int gFail = 0;

// Utility functions:
template< class T >
bool XmlTest( const char* testString, T expected, T found, bool noEcho = false )
{
	if ( expected == found ) 
		cout << "[pass]";
	else
		cout << "[fail]";

	if ( noEcho )
		cout << " " << testString;
	else
		cout << " " << testString << " [" << expected << "][" <<  found << "]";
	cout << "\n";

	bool pass = ( expected == found );
	if ( pass )
		++gPass;
	else
		++gFail;
	return pass;
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

	/*	What the todo list should look like after processing.
		In stream (no formatting) representation. */
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
		            "<Attendee name=\"Vo&#x82;\" position=\"counselor\" />"
		        "</Meeting>"
		        "<Meeting where=\"Lunch\" />"
		    "</Item>"
		    "<Item priority=\"2\" distance=\"here\">Do bills"
		    "</Item>"
		"</ToDo>";

	// The example parses from the character string (above):

	{
		// Write to a file and read it back, to check file I/O.

		TiXmlDocument doc( "demotest.xml" );
		doc.Parse( demoStart );

		if ( doc.Error() )
		{
			printf( "Error in %s: %s\n", doc.Value().c_str(), doc.ErrorDesc().c_str() );
			exit( 1 );
		}
		doc.SaveFile();
	}

	TiXmlDocument doc( "demotest.xml" );
	bool loadOkay = doc.LoadFile();

	if ( !loadOkay )
	{
		printf( "Could not load test file 'demotest.xml'. Error='%s'. Exiting.\n", doc.ErrorDesc().c_str() );
		exit( 1 );
	}

	printf( "** Demo doc read from disk: ** \n\n" );
	doc.Print( stdout );

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

	// Remove the "Look for Evil Dinosours!" item.
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
	attendee2.SetAttribute( "name", "Vo&#x82;" );
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

	printf( "** Demo doc processed to stream: ** \n\n" );
	cout << doc << endl << endl;

	// --------------------------------------------------------
	// Different tests...do we have what we expect?
	// --------------------------------------------------------

	int count = 0;
	TiXmlElement*	element;

	//////////////////////////////////////////////////////
	cout << "** Basic structure. **\n";
	ostringstream outputStream( ostringstream::out );
	outputStream << doc;

	XmlTest( "Output stream correct.", string( demoEnd ), outputStream.str(), true );

	node = doc.RootElement();
	XmlTest( "Root element exists.", true, ( node != 0 && node->ToElement() ) );	
	XmlTest( "Root element value is 'ToDo'.", string( "ToDo" ), node->Value() );
	node = node->FirstChild();
	XmlTest( "First child exists & is a comment.", true, ( node != 0 && node->ToComment() ) );
	node = node->NextSibling();
	XmlTest( "Sibling element exists & is an element.", true, ( node != 0 && node->ToElement() ) );
	XmlTest( "Value is 'Item'.", string( "Item" ), node->Value() );
	node = node->FirstChild();
	XmlTest( "First child exists.", true, ( node != 0 && node->ToText() ) );
	XmlTest( "Value is 'Go to the'.", string( "Go to the" ), node->Value() );


	//////////////////////////////////////////////////////
	cout << "\n** Iterators. **" << "\n";
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
	// using a different sytax.
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


	//////////////////////////////////////////////////////
	cout << "\n** Parsing. **\n";
	istringstream parse0( "<Element0 attribute0='foo0' attribute1= noquotes attribute2 = '&gt;' />" );
	TiXmlElement element0( "default" );
	parse0 >> element0;

	XmlTest( "Element parsed, value is 'Element0'.", string( "Element0" ), element0.Value() );
	XmlTest( "Reads attribute 'attribute0=\"foo0\"'.", string( "foo0" ), *( element0.Attribute( "attribute0" ) ) );
	XmlTest( "Reads incorrectly formatted 'attribute1=noquotes'.", string( "noquotes" ), *( element0.Attribute( "attribute1" ) ) );
	XmlTest( "Read attribute with entity value '>'.", string( ">" ), *( element0.Attribute( "attribute2" ) ) );

	//////////////////////////////////////////////////////
	cout << "\n** Streaming. **\n";

	// Round trip check: stream in, then stream back out to verify. The stream
	// out has already been checked, above. We use the output

	istringstream inputStringStream( outputStream.str() );
	TiXmlDocument document0;

	inputStringStream >> document0;

	ostringstream outputStream0( ostringstream::out );
	outputStream0 << document0;

	XmlTest( "Stream round trip correct.", string( demoEnd ), outputStream0.str(), true );

	//////////////////////////////////////////////////////
	cout << "\n** Parsing, no Condense Whitespace **\n";
	TiXmlBase::SetCondenseWhiteSpace( false );

	istringstream parse1( "<start>This  is    \ntext</start>" );
	TiXmlElement text1( "text" );
	parse1 >> text1;

	XmlTest( "Condense white space OFF.", string( "This  is    \ntext" ),
										  text1.FirstChild()->Value(),
										  true );
							
	cout << endl << "Pass " << gPass << ", Fail " << gFail << endl;	
	return 0;
}

