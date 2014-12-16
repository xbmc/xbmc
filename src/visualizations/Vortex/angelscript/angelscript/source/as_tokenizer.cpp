/*
   AngelCode Scripting Library
   Copyright (c) 2003-2009 Andreas Jonsson

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

   Andreas Jonsson
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

#if !defined(AS_NO_MEMORY_H)
#include <memory.h>
#endif
#include <string.h> // strcmp()

BEGIN_AS_NAMESPACE

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
	if( tokenType == ttMultilineStringConstant      ) return "<multiline string constant>";
	if( tokenType == ttNonTerminatedStringConstant	) return "<nonterminated string constant>";
	if( tokenType == ttBitsConstant					) return "<bits constant>";
	if( tokenType == ttHeredocStringConstant		) return "<heredoc string constant>";

	for( asUINT n = 0; n < numTokenWords; n++ )
		if( tokenWords[n].tokenType == tokenType )
			return tokenWords[n].word;

	return 0;
}

eTokenType asCTokenizer::GetToken(const char *source, size_t sourceLength, size_t *tokenLength, asETokenClass *tc)
{
	asASSERT(source != 0);
	asASSERT(tokenLength != 0);

	this->source = source;
	this->sourceLength = sourceLength;

	asETokenClass t = ParseToken();
	if( tc ) *tc = t;

	// Copy the output to the token
	*tokenLength = this->tokenLength;

	return tokenType;
}

asETokenClass asCTokenizer::ParseToken()
{
	if( IsWhiteSpace() ) return asTC_WHITESPACE;
	if( IsComment()    ) return asTC_COMMENT;
	if( IsConstant()   ) return asTC_VALUE;
	if( IsIdentifier() ) return asTC_IDENTIFIER;
	if( IsKeyWord()    ) return asTC_KEYWORD;

	// If none of the above this is an unrecognized token
	// We can find the length of the token by advancing
	// one step and trying to identify a token there
	tokenType = ttUnrecognizedToken;
	tokenLength = 1;

	return asTC_UNKNOWN;
}

bool asCTokenizer::IsWhiteSpace()
{
	// Treat UTF8 byte-order-mark (EF BB BF) as whitespace
	if( sourceLength >= 3 && 
		asBYTE(source[0]) == 0xEFu &&
		asBYTE(source[1]) == 0xBBu &&
		asBYTE(source[2]) == 0xBFu )
	{
		tokenType = ttWhiteSpace;
		tokenLength = 3;
		return true;
	}

	// Group all other white space characters into one
	size_t n;
	int numWsChars = (int)strlen(whiteSpace);
	for( n = 0; n < sourceLength; n++ )
	{
		bool isWhiteSpace = false;
		for( int w = 0; w < numWsChars; w++ )
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
		size_t n;
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
		size_t n;
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
		if( source[0] == '0' && sourceLength >= 1 && (source[1] == 'x' || source[1] == 'X') )
		{
			size_t n;
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

		size_t n;
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

	// String constant between double or single quotes
	if( source[0] == '"' || source[0] == '\'' )
	{
		// Is it a normal string constant or a heredoc string constant?
		if( sourceLength >= 6 && source[0] == '"' && source[1] == '"' && source[2] == '"' )
		{
			// Heredoc string constant (spans multiple lines, no escape sequences)

			// Find the length
			size_t n;
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
			tokenType = ttStringConstant;
			char quote = source[0];
			bool evenSlashes = true;
			size_t n;
			for( n = 1; n < sourceLength; n++ )
			{
#ifdef AS_DOUBLEBYTE_CHARSET
				// Double-byte characters are only allowed for ASCII
				if( (source[n] & 0x80) && engine->ep.scanner == 0 )
				{
					// This is a leading character in a double byte character, 
					// include both in the string and continue processing.
					n++;
					continue;
				}
#endif

				if( source[n] == '\n' ) 
					tokenType = ttMultilineStringConstant;
				if( source[n] == quote && evenSlashes )
				{
					tokenLength = n+1;
					return true;
				}
				if( source[n] == '\\' ) evenSlashes = !evenSlashes; else evenSlashes = true;
			}

			tokenType = ttNonTerminatedStringConstant;
			tokenLength = n;
		}

		return true;
	}

	return false;
}

bool asCTokenizer::IsIdentifier()
{
	// Starting with letter or underscore
	if( (source[0] >= 'a' && source[0] <= 'z') ||
		(source[0] >= 'A' && source[0] <= 'Z') ||
		source[0] == '_' )
	{
		tokenType = ttIdentifier;
		tokenLength = 1;

		for( size_t n = 1; n < sourceLength; n++ )
		{
			if( (source[n] >= 'a' && source[n] <= 'z') ||
				(source[n] >= 'A' && source[n] <= 'Z') ||
				(source[n] >= '0' && source[n] <= '9') ||
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

		for( asUINT i = 0; i < numTokenWords; i++ )
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
	asUINT n;
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
				// tokens that end with a character that can be part of an 
				// identifier require an extra verification to guarantee that 
				// we don't split an identifier token, e.g. the "!is" token 
				// and the "!isTrue" expression.
				if( ((tokenWords[words[i]].word[n-1] >= 'a' && tokenWords[words[i]].word[n-1] <= 'z') ||
					 (tokenWords[words[i]].word[n-1] >= 'A' && tokenWords[words[i]].word[n-1] <= 'Z')) &&
					((source[n] >= 'a' && source[n] <= 'z') ||
					 (source[n] >= 'A' && source[n] <= 'Z') ||
					 (source[n] >= '0' && source[n] <= '9') ||
					 (source[n] == '_')) )
				{
					// The token doesn't really match, even though 
					// the start of the source matches the token
					words[i--] = words[--numWords];
				}
				else if( numWords > 1 )
				{
					// It's possible that a longer token matches, so let's 
					// remember this match and continue searching
					lastPossible = words[i];
					words[i--] = words[--numWords];
					continue;
				}
				else
				{
					// Only one token matches, so we return it
					tokenType = tokenWords[words[i]].tokenType;
					tokenLength = n;
					return true;
				}
			}
			else if( tokenWords[words[i]].word[n] != source[n] )
			{
				// The token doesn't match
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

END_AS_NAMESPACE

