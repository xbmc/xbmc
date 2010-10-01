/*
   AngelCode Scripting Library
   Copyright (c) 2003-2006 Andreas Jönsson

   This software is provided 'as-is', without any express or implied 
   warranty. In no event will the authors be held liable for any 
   damages arising from the use of this software.

   Permission is granted to anyone to use this software for any 
   purpose, including commercial applications, and to alter it and 
   redistribute it freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you 
      must not claim that you wrote the original software. If you use
      this software in a product, an acknowledgment in the product 
      documentation would be appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and 
      must not be misrepresented as being the original software.

   3. This notice may not be removed or altered from any source 
      distribution.

   The original version of this library can be located at:
   http://www.angelcode.com/angelscript/

   Andreas Jönsson
   andreas@angelcode.com
*/


//
// as_tokenizer.cpp
//
// This class identifies tokens from the script code
//


#include "as_config.h"
#include "as_tokenizer.h"
#include "as_tokendef.h"

#include <assert.h> // assert()
#include <memory.h> // memcpy()
#include <string.h> // strcmp()

asCTokenizer::asCTokenizer()
{
}

asCTokenizer::~asCTokenizer()
{
}

const char *asGetTokenDefinition(int tokenType)
{
	if( tokenType == ttUnrecognizedToken			) return "<unrecognized token>";
	if( tokenType == ttEnd							) return "<end of file>";
	if( tokenType == ttWhiteSpace					) return "<white space>";
	if( tokenType == ttOnelineComment				) return "<one line comment>";
	if( tokenType == ttMultilineComment				) return "<multiple lines comment>";
	if( tokenType == ttIdentifier					) return "<identifier>";
	if( tokenType == ttIntConstant					) return "<integer constant>";
	if( tokenType == ttFloatConstant				) return "<float constant>";
	if( tokenType == ttDoubleConstant				) return "<double constant>";
	if( tokenType == ttStringConstant				) return "<string constant>";
	if( tokenType == ttNonTerminatedStringConstant	) return "<unterminated string constant>";
	if( tokenType == ttBitsConstant					) return "<bits constant>";
	if( tokenType == ttHeredocStringConstant		) return "<heredoc string constant>";

	for( int n = 0; n < numTokenWords; n++ )
		if( tokenWords[n].tokenType == tokenType )
			return tokenWords[n].word;

	return 0;
}

eTokenType asCTokenizer::GetToken(const char *source, int sourceLength, int *tokenLength)
{
	assert(source != 0);
	assert(tokenLength != 0);

	this->source = source;
	this->sourceLength = sourceLength;

	ParseToken();

	// Copy the output to the token
	*tokenLength = this->tokenLength;

	return tokenType;
}

int asCTokenizer::ParseToken()
{
	if( IsWhiteSpace() ) return 0;
	if( IsComment()    ) return 0;
	if( IsConstant()   ) return 0;
	if( IsIdentifier() ) return 0;
	if( IsKeyWord()    ) return 0;

	// If none of the above this is an unrecognized token
	// We can find the length of the token by advancing
	// one step and trying to identify a token there
	tokenType = ttUnrecognizedToken;
	tokenLength = 1;

	return -1;
}

bool asCTokenizer::IsWhiteSpace()
{
	int n;
	for( n = 0; n < sourceLength; n++ )
	{
		bool isWhiteSpace = false;
		for( int w = 0; w < (int)sizeof(whiteSpace); w++ )
		{
			if( source[n] == whiteSpace[w] )
			{
				isWhiteSpace = true;
				break;
			}
		}
		if( !isWhiteSpace )	break;
	}

	if( n > 0 )
	{
		tokenType = ttWhiteSpace;
		tokenLength = n;
		return true;
	}

	return false;
}

bool asCTokenizer::IsComment()
{
	if( sourceLength < 2 )
		return false;

	if( source[0] != '/' )
		return false;

	if( source[1] == '/' )
	{
		// One-line comment

		// Find the length
		int n;
		for( n = 2; n < sourceLength; n++ )
		{
			if( source[n] == '\n' )
				break;
		}

		tokenType = ttOnelineComment;
		tokenLength = n+1;

		return true;
	}

	if( source[1] == '*' )
	{
		// Multi-line comment

		// Find the length
		int n;
		for( n = 2; n < sourceLength-1; )
		{
			if( source[n++] == '*' && source[n] == '/' )
				break;
		}

		tokenType = ttMultilineComment;
		tokenLength = n+1;

		return true;
	}

	return false;
}

bool asCTokenizer::IsConstant()
{
	// Starting with number
	if( source[0] >= '0' && source[0] <= '9' )
	{
		// Is it a hexadecimal number?
		if( sourceLength >= 1 && (source[1] == 'x' || source[1] == 'X') )
		{
			int n;
			for( n = 2; n < sourceLength; n++ )
			{
				if( !(source[n] >= '0' && source[n] <= '9') &&
					!(source[n] >= 'a' && source[n] <= 'f') &&
					!(source[n] >= 'A' && source[n] <= 'F') )
					break;
			}

			tokenType = ttBitsConstant;
			tokenLength = n;
			return true;
		}

		int n;
		for( n = 1; n < sourceLength; n++ )
		{
			if( source[n] < '0' || source[n] > '9' )
				break;
		}

		if( n < sourceLength && source[n] == '.' )
		{
			n++;
			for( ; n < sourceLength; n++ )
			{
				if( source[n] < '0' || source[n] > '9' )
					break;
			}

			if( n < sourceLength && (source[n] == 'e' || source[n] == 'E') )
			{
				n++;
				if( n < sourceLength && (source[n] == '-' || source[n] == '+') )
					n++;

				for( ; n < sourceLength; n++ )
				{
					if( source[n] < '0' || source[n] > '9' )
						break;
				}
			}

			if( n < sourceLength && (source[n] == 'f' || source[n] == 'F') )
			{
				tokenType = ttFloatConstant;
				tokenLength = n + 1;
			}
			else
			{
#ifdef AS_USE_DOUBLE_AS_FLOAT
				tokenType = ttFloatConstant;
#else
				tokenType = ttDoubleConstant;
#endif
				tokenLength = n;
			}
			return true;
		}

		tokenType = ttIntConstant;
		tokenLength = n;
		return true;
	}

	// Character literal between single-quotes
	if( source[0] == '\'' )
	{
		bool evenSlashes = true;
		int n;
		for( n = 1; n < sourceLength; n++ )
		{
			if( source[n] == '\n' ) break;
			if( source[n] == '\'' && evenSlashes )
			{
				tokenType = ttIntConstant;
				tokenLength = n+1;
				return true;
			}
			if( source[n] == '\\' ) evenSlashes = !evenSlashes; else evenSlashes = true;
		}

		tokenType = ttNonTerminatedStringConstant;
		tokenLength = n-1;

		return true;
	}

	// String constant between double-quotes
	if( source[0] == '"' )
	{
		// Is it a normal string constant or a heredoc string constant?
		if( sourceLength >= 6 && source[1] == '"' && source[2] == '"' )
		{
			// Heredoc string constant (spans multiple lines, no escape sequences)

			// Find the length
			int n;
			for( n = 3; n < sourceLength-2; n++ )
			{
				if( source[n] == '"' && source[n+1] == '"' && source[n+2] == '"' )
					break;
			}

			tokenType = ttHeredocStringConstant;
			tokenLength = n+3;
		}
		else
		{
			// Normal string constant
			bool evenSlashes = true;
			int n;
			for( n = 1; n < sourceLength; n++ )
			{
				if( source[n] == '\n' ) break;
				if( source[n] == '"' && evenSlashes )
				{
					tokenType = ttStringConstant;
					tokenLength = n+1;
					return true;
				}
				if( source[n] == '\\' ) evenSlashes = !evenSlashes; else evenSlashes = true;
			}

			tokenType = ttNonTerminatedStringConstant;
			tokenLength = n-1;
		}

		return true;
	}

	return false;
}

bool asCTokenizer::IsIdentifier()
{
	// Starting with letter or underscore
	if( source[0] >= 'a' && source[0] <= 'z' ||
		source[0] >= 'A' && source[0] <= 'Z' ||
		source[0] == '_' )
	{
		tokenType = ttIdentifier;
		tokenLength = 1;

		for( int n = 1; n < sourceLength; n++ )
		{
			if( source[n] >= 'a' && source[n] <= 'z' ||
				source[n] >= 'A' && source[n] <= 'Z' ||
				source[n] >= '0' && source[n] <= '9' ||
				source[n] == '_' )
				tokenLength++;
			else
				break;
		}

		// Make sure the identifier isn't a reserved keyword
		if( tokenLength > 50 ) return true;

		char test[51];
		memcpy(test, source, tokenLength);
		test[tokenLength] = 0;

		for( int i = 0; i < numTokenWords; i++ )
		{
			if( strcmp(test, tokenWords[i].word) == 0 )
				return false;
		}

		return true;
	}

	return false;
}

bool asCTokenizer::IsKeyWord()
{
	// Fill the list with all possible keywords
	// Check each character against all the keywords in the list,
	// remove keywords that don't match. When only one remains and
	// it matches the source completely we have found a match.
	int words[numTokenWords];
	int n;
	for( n = 0; n < numTokenWords; n++ )
		words[n] = n;

	int numWords = numTokenWords;
	int lastPossible = -1;

	n = 0;
	while( n < sourceLength && numWords >= 0 )
	{
		for( int i = 0; i < numWords; i++ )
		{
			if( tokenWords[words[i]].word[n] == '\0' )
			{
				if( numWords > 1 )
				{
					lastPossible = words[i];
					words[i--] = words[--numWords];
					continue;
				}
				else
				{
					tokenType = tokenWords[words[i]].tokenType;
					tokenLength = n;
					return true;
				}
			}

			if( tokenWords[words[i]].word[n] != source[n] )
			{
				words[i--] = words[--numWords];
			}
		}
		n++;
	}

	// The source length ended or there where no more matchable words
	if( numWords )
	{
		// If any of the tokenWords also end at this
		// position then we have found the matching token
		for( int i = 0; i < numWords; i++ )
		{
			if( tokenWords[words[i]].word[n] == '\0' )
			{
				tokenType = tokenWords[words[i]].tokenType;
				tokenLength = n;
				return true;
			}
		}
	}

	// It is still possible that a shorter token was found
	if( lastPossible > -1 )
	{
		tokenType = tokenWords[lastPossible].tokenType;
		tokenLength = strlen(tokenWords[lastPossible].word);
		return true;
	}

	return false;
}


