#ifndef XISTREAMREADER_H
#define XISTREAMREADER_H

#pragma once

class ID3_XIStreamReader : public ID3_Reader  
{
 
public:
		ID3_XIStreamReader(CFile& reader) : _stream( reader ) { ; }
		virtual ~ID3_XIStreamReader() { ; }
		
		/** Close the reader.  Any further actions on the reader should fail.**/
  	virtual void close() 
		{ 
			; 
		}
  
		/**
		** Return the next character to be read without advancing the internal 
		** position.  Returns END_OF_READER if there isn't a character to read.
		**/
  	virtual int_type peekChar() 
		{
			if (this->atEnd())
			{ 
				return END_OF_READER; 
			}
			int_type buf = 0;
			__int64 i = _stream.GetPosition();

			buf = (int_type)this->readChar();
			_stream.Seek( i );
			return buf ; 
		}
    
		/** Read up to \c len chars into buf and advance the internal position
		** accordingly.  Returns the number of characters read into buf.
		**/
	
		virtual size_type readChars(char_type buf[], size_type len)
		{
			if (this->atEnd())
			{ 
				return END_OF_READER; 
			}
			__int64 pt = _stream.GetPosition();
			size_type st = (_stream.Read((void*) buf, (int) len));
			_stream.Seek( pt + st );
			return st;
		}

		/** Return the beginning position in the reader */
		virtual pos_type getBeg() { return 0; }
		
		/** Return the current position in the reader */
		virtual pos_type getCur() { return pos_type(_stream.GetPosition()); }
		
		/** Return the ending position in the reader */
		virtual pos_type getEnd() 
		{
			return (ID3_Reader::pos_type)_stream.GetLength();
		}
    
		/** Set the value of the current position for reading.**/
		virtual pos_type setCur(pos_type pos) 
		{ 
			_stream.Seek(pos); 
			return pos_type(pos); 
		}
	
		/** Skip up to \c len chars in the stream and advance the internal position
   ** accordingly.  Returns the number of characters actually skipped (may be 
   ** less than requested).
   
		virtual size_type skipChars(size_type len)
		{
			const size_type SIZE = 1024;
			char_type* bytes;
			size_type remaining = len;
			if ( getCur() + len > getEnd() ) 
			{
				bytes = new char_type[getEnd() - getCur()];
				this->readChars(bytes, getEnd() - getCur() );
				delete bytes;
				return (getCur() + len) - getEnd();
			}
			else 
			{
				bytes = new char_type[len];
				this->readChars(bytes, len );
				delete bytes;
				return 0;
			}
			return len - remaining;
		}**/

 protected:
		CFile& _stream;
		CFile& getReader() const { return _stream; }

};
#endif