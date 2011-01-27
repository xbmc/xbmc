/* -----------------------------------------------------------------------------

	Copyright (c) 2006 Simon Brown                          si@sjbrown.co.uk

	Permission is hereby granted, free of charge, to any person obtaining
	a copy of this software and associated documentation files (the 
	"Software"), to	deal in the Software without restriction, including
	without limitation the rights to use, copy, modify, merge, publish,
	distribute, sublicense, and/or sell copies of the Software, and to 
	permit persons to whom the Software is furnished to do so, subject to 
	the following conditions:

	The above copyright notice and this permission notice shall be included
	in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
	OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
	IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY 
	CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
	TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
	SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
	
   -------------------------------------------------------------------------- */
   
#include <iostream>

struct SourceBlock
{
	int start;
	int end;
	int error;
};

struct TargetValue
{
	SourceBlock sources[2];
};

static void GenerateData( std::string const& name, int bits, int colours )
{
	TargetValue values[256];
	
	// initialise the data
	for( int target = 0; target < 256; ++target )
		for( int index = 0; index < colours; ++index )
			values[target].sources[index].error = 255;	

	// loop over all possible source points
	int count = ( 1 << bits );
	for( int value1 = 0; value1 < count; ++value1 )
	{
		for( int value2 = 0; value2 < count; ++value2 )
		{
			// compute the 8-bit endpoints
			int a = ( value1 << ( 8 - bits ) ) | ( value1 >> ( 2*bits - 8 ) );
			int b = ( value2 << ( 8 - bits ) ) | ( value2 >> ( 2*bits - 8 ) );
			
			// fill in the codebook with the these and intermediates
			int codes[2];
			codes[0] = a;
			if( colours == 3 )
				codes[1] = ( a + b )/2;
			else
				codes[1] = ( 2*a + b )/3;
			
			// mark each target point with the endpoints and index needed for it
			for( int index = 0; index < 2; ++index )
			{
				int target = codes[index];
				
				SourceBlock& block = values[target].sources[index];
				if( block.error != 0 )
				{
					block.start = value1;
					block.end = value2;
					block.error = 0;
				}
			}
		}
	}
	
	// iteratively fill in the missing values
	for( ;; )
	{
		bool stable = true;
		for( int index = 0; index < 2; ++index )
		{
			for( int target = 0; target < 256; ++target )
			{
				if( target != 255 )
				{
					SourceBlock& current = values[target].sources[index];
					SourceBlock& next = values[target + 1].sources[index];
					if( current.error > next.error + 1 )
					{
						current.start = next.start;
						current.end = next.end;
						current.error = next.error + 1;
						stable = false;
					}
				}
				if( target != 0 )
				{
					SourceBlock& current = values[target].sources[index];
					SourceBlock& previous = values[target - 1].sources[index];
					if( current.error > previous.error + 1 )
					{
						current.start = previous.start;
						current.end = previous.end;
						current.error = previous.error + 1;
						stable = false;
					}
				}
			}
		}
		if( stable )
			break;
	}
	
	// debug
	std::cout << "\nstatic SingleColourLookup const " << name << "[] = \n{\n"; 
	for( int i = 0;; )
	{
		std::cout << "\t{ { ";
		for( int j = 0;; )
		{
			SourceBlock const& block = values[i].sources[j];
			if( j < colours )
				std::cout << "{ " << block.start << ", " << block.end << ", " << block.error << " }";
			else
				std::cout << "{ 0, 0, 0 }";
			if( ++j == 2 )
				break;
			std::cout << ", ";
		}
		std::cout << " } }";
		if( ++i == 256 )
			break;
		std::cout << ",\n";
	}
	std::cout << "\n};\n";
}

int main()
{
	GenerateData( "lookup_5_3", 5, 3 );
	GenerateData( "lookup_6_3", 6, 3 );
	GenerateData( "lookup_5_4", 5, 4 );
	GenerateData( "lookup_6_4", 6, 4 );
}
