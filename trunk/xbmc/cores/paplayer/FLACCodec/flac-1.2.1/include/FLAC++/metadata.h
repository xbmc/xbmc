/* libFLAC++ - Free Lossless Audio Codec library
 * Copyright (C) 2002,2003,2004,2005,2006,2007  Josh Coalson
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * - Neither the name of the Xiph.org Foundation nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef FLACPP__METADATA_H
#define FLACPP__METADATA_H

#include "export.h"

#include "FLAC/metadata.h"

// ===============================================================
//
//  Full documentation for the metadata interface can be found
//  in the C layer in include/FLAC/metadata.h
//
// ===============================================================

/** \file include/FLAC++/metadata.h
 *
 *  \brief
 *  This module provides classes for creating and manipulating FLAC
 *  metadata blocks in memory, and three progressively more powerful
 *  interfaces for traversing and editing metadata in FLAC files.
 *
 *  See the detailed documentation for each interface in the
 *  \link flacpp_metadata metadata \endlink module.
 */

/** \defgroup flacpp_metadata FLAC++/metadata.h: metadata interfaces
 *  \ingroup flacpp
 *
 *  \brief
 *  This module provides classes for creating and manipulating FLAC
 *  metadata blocks in memory, and three progressively more powerful
 *  interfaces for traversing and editing metadata in FLAC files.
 *
 *  The behavior closely mimics the C layer interface; be sure to read
 *  the detailed description of the
 *  \link flac_metadata C metadata module \endlink.  Note that like the
 *  C layer, currently only the Chain interface (level 2) supports Ogg
 *  FLAC files, and it is read-only i.e. no writing back changed
 *  metadata to file.
 */


namespace FLAC {
	namespace Metadata {

		// ============================================================
		//
		//  Metadata objects
		//
		// ============================================================

		/** \defgroup flacpp_metadata_object FLAC++/metadata.h: metadata object classes
		 *  \ingroup flacpp_metadata
		 *
		 * This module contains classes representing FLAC metadata
		 * blocks in memory.
		 *
		 * The behavior closely mimics the C layer interface; be
		 * sure to read the detailed description of the
		 * \link flac_metadata_object C metadata object module \endlink.
		 *
		 * Any time a metadata object is constructed or assigned, you
		 * should check is_valid() to make sure the underlying
		 * ::FLAC__StreamMetadata object was able to be created.
		 *
		 * \warning
		 * When the get_*() methods of any metadata object method
		 * return you a const pointer, DO NOT disobey and write into it.
		 * Always use the set_*() methods.
		 *
		 * \{
		 */

		/** Base class for all metadata block types.
		 *  See the \link flacpp_metadata_object overview \endlink for more.
		 */
		class FLACPP_API Prototype {
		protected:
			//@{
			/** Constructs a copy of the given object.  This form
			 *  always performs a deep copy.
			 */
			Prototype(const Prototype &);
			Prototype(const ::FLAC__StreamMetadata &);
			Prototype(const ::FLAC__StreamMetadata *);
			//@}

			/** Constructs an object with copy control.  When \a copy
			 *  is \c true, behaves identically to
			 *  FLAC::Metadata::Prototype::Prototype(const ::FLAC__StreamMetadata *object).
			 *  When \a copy is \c false, the instance takes ownership of
			 *  the pointer and the ::FLAC__StreamMetadata object will
			 *  be freed by the destructor.
			 *
			 *  \assert
			 *    \code object != NULL \endcode
			 */
			Prototype(::FLAC__StreamMetadata *object, bool copy);

			//@{
			/** Assign from another object.  Always performs a deep copy. */
			Prototype &operator=(const Prototype &);
			Prototype &operator=(const ::FLAC__StreamMetadata &);
			Prototype &operator=(const ::FLAC__StreamMetadata *);
			//@}

			/** Assigns an object with copy control.  See
			 *  Prototype(::FLAC__StreamMetadata *object, bool copy).
			 */
			Prototype &assign_object(::FLAC__StreamMetadata *object, bool copy);

			/** Deletes the underlying ::FLAC__StreamMetadata object.
			 */
			virtual void clear();

			::FLAC__StreamMetadata *object_;
		public:
			/** Deletes the underlying ::FLAC__StreamMetadata object.
			 */
			virtual ~Prototype();

			//@{
			/** Check for equality, performing a deep compare by following pointers.
			 */
			inline bool operator==(const Prototype &) const;
			inline bool operator==(const ::FLAC__StreamMetadata &) const;
			inline bool operator==(const ::FLAC__StreamMetadata *) const;
			//@}

			//@{
			/** Check for inequality, performing a deep compare by following pointers. */
			inline bool operator!=(const Prototype &) const;
			inline bool operator!=(const ::FLAC__StreamMetadata &) const;
			inline bool operator!=(const ::FLAC__StreamMetadata *) const;
			//@}

			friend class SimpleIterator;
			friend class Iterator;

			/** Returns \c true if the object was correctly constructed
			 *  (i.e. the underlying ::FLAC__StreamMetadata object was
			 *  properly allocated), else \c false.
			 */
			inline bool is_valid() const;

			/** Returns \c true if this block is the last block in a
			 *  stream, else \c false.
			 *
			 * \assert
			 *   \code is_valid() \endcode
			 */
			bool get_is_last() const;

			/** Returns the type of the block.
			 *
			 * \assert
			 *   \code is_valid() \endcode
			 */
			::FLAC__MetadataType get_type() const;

			/** Returns the stream length of the metadata block.
			 *
			 * \note
			 *   The length does not include the metadata block header,
			 *   per spec.
			 *
			 * \assert
			 *   \code is_valid() \endcode
			 */
			unsigned get_length() const;

			/** Sets the "is_last" flag for the block.  When using the iterators
			 *  it is not necessary to set this flag; they will do it for you.
			 *
			 * \assert
			 *   \code is_valid() \endcode
			 */
			void set_is_last(bool);

			/** Returns a pointer to the underlying ::FLAC__StreamMetadata
			 *  object.  This can be useful for plugging any holes between
			 *  the C++ and C interfaces.
			 *
			 * \assert
			 *   \code is_valid() \endcode
			 */
			inline operator const ::FLAC__StreamMetadata *() const;
		private:
			/** Private and undefined so you can't use it. */
			Prototype();

			// These are used only by Iterator
			bool is_reference_;
			inline void set_reference(bool x) { is_reference_ = x; }
		};

#ifdef _MSC_VER
// warning C4800: 'int' : forcing to bool 'true' or 'false' (performance warning)
#pragma warning ( disable : 4800 )
#endif

		inline bool Prototype::operator==(const Prototype &object) const
		{ return (bool)::FLAC__metadata_object_is_equal(object_, object.object_); }

		inline bool Prototype::operator==(const ::FLAC__StreamMetadata &object) const
		{ return (bool)::FLAC__metadata_object_is_equal(object_, &object); }

		inline bool Prototype::operator==(const ::FLAC__StreamMetadata *object) const
		{ return (bool)::FLAC__metadata_object_is_equal(object_, object); }

#ifdef _MSC_VER
// @@@ how to re-enable?  the following doesn't work
// #pragma warning ( enable : 4800 )
#endif

		inline bool Prototype::operator!=(const Prototype &object) const
		{ return !operator==(object); }

		inline bool Prototype::operator!=(const ::FLAC__StreamMetadata &object) const
		{ return !operator==(object); }

		inline bool Prototype::operator!=(const ::FLAC__StreamMetadata *object) const
		{ return !operator==(object); }

		inline bool Prototype::is_valid() const
		{ return 0 != object_; }

		inline Prototype::operator const ::FLAC__StreamMetadata *() const
		{ return object_; }

		/** Create a deep copy of an object and return it. */
		FLACPP_API Prototype *clone(const Prototype *);


		/** STREAMINFO metadata block.
		 *  See the \link flacpp_metadata_object overview \endlink for more,
		 *  and the <A HREF="../format.html#metadata_block_streaminfo">format specification</A>.
		 */
		class FLACPP_API StreamInfo : public Prototype {
		public:
			StreamInfo();

			//@{
			/** Constructs a copy of the given object.  This form
			 *  always performs a deep copy.
			 */
			inline StreamInfo(const StreamInfo &object): Prototype(object) { }
			inline StreamInfo(const ::FLAC__StreamMetadata &object): Prototype(object) { }
			inline StreamInfo(const ::FLAC__StreamMetadata *object): Prototype(object) { }
			//@}

			/** Constructs an object with copy control.  See
			 *  Prototype(::FLAC__StreamMetadata *object, bool copy).
			 */
			inline StreamInfo(::FLAC__StreamMetadata *object, bool copy): Prototype(object, copy) { }

			~StreamInfo();

			//@{
			/** Assign from another object.  Always performs a deep copy. */
			inline StreamInfo &operator=(const StreamInfo &object) { Prototype::operator=(object); return *this; }
			inline StreamInfo &operator=(const ::FLAC__StreamMetadata &object) { Prototype::operator=(object); return *this; }
			inline StreamInfo &operator=(const ::FLAC__StreamMetadata *object) { Prototype::operator=(object); return *this; }
			//@}

			/** Assigns an object with copy control.  See
			 *  Prototype::assign_object(::FLAC__StreamMetadata *object, bool copy).
			 */
			inline StreamInfo &assign(::FLAC__StreamMetadata *object, bool copy) { Prototype::assign_object(object, copy); return *this; }

			//@{
			/** Check for equality, performing a deep compare by following pointers. */
			inline bool operator==(const StreamInfo &object) const { return Prototype::operator==(object); }
			inline bool operator==(const ::FLAC__StreamMetadata &object) const { return Prototype::operator==(object); }
			inline bool operator==(const ::FLAC__StreamMetadata *object) const { return Prototype::operator==(object); }
			//@}

			//@{
			/** Check for inequality, performing a deep compare by following pointers. */
			inline bool operator!=(const StreamInfo &object) const { return Prototype::operator!=(object); }
			inline bool operator!=(const ::FLAC__StreamMetadata &object) const { return Prototype::operator!=(object); }
			inline bool operator!=(const ::FLAC__StreamMetadata *object) const { return Prototype::operator!=(object); }
			//@}

			//@{
			/** See <A HREF="../format.html#metadata_block_streaminfo">format specification</A>. */
			unsigned get_min_blocksize() const;
			unsigned get_max_blocksize() const;
			unsigned get_min_framesize() const;
			unsigned get_max_framesize() const;
			unsigned get_sample_rate() const;
			unsigned get_channels() const;
			unsigned get_bits_per_sample() const;
			FLAC__uint64 get_total_samples() const;
			const FLAC__byte *get_md5sum() const;

			void set_min_blocksize(unsigned value);
			void set_max_blocksize(unsigned value);
			void set_min_framesize(unsigned value);
			void set_max_framesize(unsigned value);
			void set_sample_rate(unsigned value);
			void set_channels(unsigned value);
			void set_bits_per_sample(unsigned value);
			void set_total_samples(FLAC__uint64 value);
			void set_md5sum(const FLAC__byte value[16]);
			//@}
		};

		/** PADDING metadata block.
		 *  See the \link flacpp_metadata_object overview \endlink for more,
		 *  and the <A HREF="../format.html#metadata_block_padding">format specification</A>.
		 */
		class FLACPP_API Padding : public Prototype {
		public:
			Padding();

			//@{
			/** Constructs a copy of the given object.  This form
			 *  always performs a deep copy.
			 */
			inline Padding(const Padding &object): Prototype(object) { }
			inline Padding(const ::FLAC__StreamMetadata &object): Prototype(object) { }
			inline Padding(const ::FLAC__StreamMetadata *object): Prototype(object) { }
			//@}

			/** Constructs an object with copy control.  See
			 *  Prototype(::FLAC__StreamMetadata *object, bool copy).
			 */
			inline Padding(::FLAC__StreamMetadata *object, bool copy): Prototype(object, copy) { }

			~Padding();

			//@{
			/** Assign from another object.  Always performs a deep copy. */
			inline Padding &operator=(const Padding &object) { Prototype::operator=(object); return *this; }
			inline Padding &operator=(const ::FLAC__StreamMetadata &object) { Prototype::operator=(object); return *this; }
			inline Padding &operator=(const ::FLAC__StreamMetadata *object) { Prototype::operator=(object); return *this; }
			//@}

			/** Assigns an object with copy control.  See
			 *  Prototype::assign_object(::FLAC__StreamMetadata *object, bool copy).
			 */
			inline Padding &assign(::FLAC__StreamMetadata *object, bool copy) { Prototype::assign_object(object, copy); return *this; }

			//@{
			/** Check for equality, performing a deep compare by following pointers. */
			inline bool operator==(const Padding &object) const { return Prototype::operator==(object); }
			inline bool operator==(const ::FLAC__StreamMetadata &object) const { return Prototype::operator==(object); }
			inline bool operator==(const ::FLAC__StreamMetadata *object) const { return Prototype::operator==(object); }
			//@}

			//@{
			/** Check for inequality, performing a deep compare by following pointers. */
			inline bool operator!=(const Padding &object) const { return Prototype::operator!=(object); }
			inline bool operator!=(const ::FLAC__StreamMetadata &object) const { return Prototype::operator!=(object); }
			inline bool operator!=(const ::FLAC__StreamMetadata *object) const { return Prototype::operator!=(object); }
			//@}

			void set_length(unsigned length);
		};

		/** APPLICATION metadata block.
		 *  See the \link flacpp_metadata_object overview \endlink for more,
		 *  and the <A HREF="../format.html#metadata_block_application">format specification</A>.
		 */
		class FLACPP_API Application : public Prototype {
		public:
			Application();
			//
			//@{
			/** Constructs a copy of the given object.  This form
			 *  always performs a deep copy.
			 */
			inline Application(const Application &object): Prototype(object) { }
			inline Application(const ::FLAC__StreamMetadata &object): Prototype(object) { }
			inline Application(const ::FLAC__StreamMetadata *object): Prototype(object) { }
			//@}

			/** Constructs an object with copy control.  See
			 *  Prototype(::FLAC__StreamMetadata *object, bool copy).
			 */
			inline Application(::FLAC__StreamMetadata *object, bool copy): Prototype(object, copy) { }

			~Application();

			//@{
			/** Assign from another object.  Always performs a deep copy. */
			inline Application &operator=(const Application &object) { Prototype::operator=(object); return *this; }
			inline Application &operator=(const ::FLAC__StreamMetadata &object) { Prototype::operator=(object); return *this; }
			inline Application &operator=(const ::FLAC__StreamMetadata *object) { Prototype::operator=(object); return *this; }
			//@}

			/** Assigns an object with copy control.  See
			 *  Prototype::assign_object(::FLAC__StreamMetadata *object, bool copy).
			 */
			inline Application &assign(::FLAC__StreamMetadata *object, bool copy) { Prototype::assign_object(object, copy); return *this; }

			//@{
			/** Check for equality, performing a deep compare by following pointers. */
			inline bool operator==(const Application &object) const { return Prototype::operator==(object); }
			inline bool operator==(const ::FLAC__StreamMetadata &object) const { return Prototype::operator==(object); }
			inline bool operator==(const ::FLAC__StreamMetadata *object) const { return Prototype::operator==(object); }
			//@}

			//@{
			/** Check for inequality, performing a deep compare by following pointers. */
			inline bool operator!=(const Application &object) const { return Prototype::operator!=(object); }
			inline bool operator!=(const ::FLAC__StreamMetadata &object) const { return Prototype::operator!=(object); }
			inline bool operator!=(const ::FLAC__StreamMetadata *object) const { return Prototype::operator!=(object); }
			//@}

			const FLAC__byte *get_id() const;
			const FLAC__byte *get_data() const;

			void set_id(const FLAC__byte value[4]);
			//! This form always copies \a data
			bool set_data(const FLAC__byte *data, unsigned length);
			bool set_data(FLAC__byte *data, unsigned length, bool copy);
		};

		/** SEEKTABLE metadata block.
		 *  See the \link flacpp_metadata_object overview \endlink for more,
		 *  and the <A HREF="../format.html#metadata_block_seektable">format specification</A>.
		 */
		class FLACPP_API SeekTable : public Prototype {
		public:
			SeekTable();

			//@{
			/** Constructs a copy of the given object.  This form
			 *  always performs a deep copy.
			 */
			inline SeekTable(const SeekTable &object): Prototype(object) { }
			inline SeekTable(const ::FLAC__StreamMetadata &object): Prototype(object) { }
			inline SeekTable(const ::FLAC__StreamMetadata *object): Prototype(object) { }
			//@}

			/** Constructs an object with copy control.  See
			 *  Prototype(::FLAC__StreamMetadata *object, bool copy).
			 */
			inline SeekTable(::FLAC__StreamMetadata *object, bool copy): Prototype(object, copy) { }

			~SeekTable();

			//@{
			/** Assign from another object.  Always performs a deep copy. */
			inline SeekTable &operator=(const SeekTable &object) { Prototype::operator=(object); return *this; }
			inline SeekTable &operator=(const ::FLAC__StreamMetadata &object) { Prototype::operator=(object); return *this; }
			inline SeekTable &operator=(const ::FLAC__StreamMetadata *object) { Prototype::operator=(object); return *this; }
			//@}

			/** Assigns an object with copy control.  See
			 *  Prototype::assign_object(::FLAC__StreamMetadata *object, bool copy).
			 */
			inline SeekTable &assign(::FLAC__StreamMetadata *object, bool copy) { Prototype::assign_object(object, copy); return *this; }

			//@{
			/** Check for equality, performing a deep compare by following pointers. */
			inline bool operator==(const SeekTable &object) const { return Prototype::operator==(object); }
			inline bool operator==(const ::FLAC__StreamMetadata &object) const { return Prototype::operator==(object); }
			inline bool operator==(const ::FLAC__StreamMetadata *object) const { return Prototype::operator==(object); }
			//@}

			//@{
			/** Check for inequality, performing a deep compare by following pointers. */
			inline bool operator!=(const SeekTable &object) const { return Prototype::operator!=(object); }
			inline bool operator!=(const ::FLAC__StreamMetadata &object) const { return Prototype::operator!=(object); }
			inline bool operator!=(const ::FLAC__StreamMetadata *object) const { return Prototype::operator!=(object); }
			//@}

			unsigned get_num_points() const;
			::FLAC__StreamMetadata_SeekPoint get_point(unsigned index) const;

			//! See FLAC__metadata_object_seektable_set_point()
			void set_point(unsigned index, const ::FLAC__StreamMetadata_SeekPoint &point);

			//! See FLAC__metadata_object_seektable_insert_point()
			bool insert_point(unsigned index, const ::FLAC__StreamMetadata_SeekPoint &point);

			//! See FLAC__metadata_object_seektable_delete_point()
			bool delete_point(unsigned index);

			//! See FLAC__metadata_object_seektable_is_legal()
			bool is_legal() const;
		};

		/** VORBIS_COMMENT metadata block.
		 *  See the \link flacpp_metadata_object overview \endlink for more,
		 *  and the <A HREF="../format.html#metadata_block_vorbis_comment">format specification</A>.
		 */
		class FLACPP_API VorbisComment : public Prototype {
		public:
			/** Convenience class for encapsulating Vorbis comment
			 *  entries.  An entry is a vendor string or a comment
			 *  field.  In the case of a vendor string, the field
			 *  name is undefined; only the field value is relevant.
			 *
			 *  A \a field as used in the methods refers to an
			 *  entire 'NAME=VALUE' string; for convenience the
			 *  string is NUL-terminated.  A length field is
			 *  required in the unlikely event that the value
			 *  contains contain embedded NULs.
			 *
			 *  A \a field_name is what is on the left side of the
			 *  first '=' in the \a field.  By definition it is ASCII
			 *  and so is NUL-terminated and does not require a
			 *  length to describe it.  \a field_name is undefined
			 *  for a vendor string entry.
			 *
			 *  A \a field_value is what is on the right side of the
			 *  first '=' in the \a field.  By definition, this may
			 *  contain embedded NULs and so a \a field_value_length
			 *  is required to describe it.  However in practice,
			 *  embedded NULs are not known to be used, so it is
			 *  generally safe to treat field values as NUL-
			 *  terminated UTF-8 strings.
			 *
			 *  Always check is_valid() after the constructor or operator=
			 *  to make sure memory was properly allocated and that the
			 *  Entry conforms to the Vorbis comment specification.
			 */
			class FLACPP_API Entry {
			public:
				Entry();

				Entry(const char *field, unsigned field_length);
				Entry(const char *field); // assumes \a field is NUL-terminated

				Entry(const char *field_name, const char *field_value, unsigned field_value_length);
				Entry(const char *field_name, const char *field_value); // assumes \a field_value is NUL-terminated

				Entry(const Entry &entry);

				Entry &operator=(const Entry &entry);

				virtual ~Entry();

				virtual bool is_valid() const; ///< Returns \c true iff object was properly constructed.

				unsigned get_field_length() const;
				unsigned get_field_name_length() const;
				unsigned get_field_value_length() const;

				::FLAC__StreamMetadata_VorbisComment_Entry get_entry() const;
				const char *get_field() const;
				const char *get_field_name() const;
				const char *get_field_value() const;

				bool set_field(const char *field, unsigned field_length);
				bool set_field(const char *field); // assumes \a field is NUL-terminated
				bool set_field_name(const char *field_name);
				bool set_field_value(const char *field_value, unsigned field_value_length);
				bool set_field_value(const char *field_value); // assumes \a field_value is NUL-terminated
			protected:
				bool is_valid_;
				::FLAC__StreamMetadata_VorbisComment_Entry entry_;
				char *field_name_;
				unsigned field_name_length_;
				char *field_value_;
				unsigned field_value_length_;
			private:
				void zero();
				void clear();
				void clear_entry();
				void clear_field_name();
				void clear_field_value();
				void construct(const char *field, unsigned field_length);
				void construct(const char *field); // assumes \a field is NUL-terminated
				void construct(const char *field_name, const char *field_value, unsigned field_value_length);
				void construct(const char *field_name, const char *field_value); // assumes \a field_value is NUL-terminated
				void compose_field();
				void parse_field();
			};

			VorbisComment();

			//@{
			/** Constructs a copy of the given object.  This form
			 *  always performs a deep copy.
			 */
			inline VorbisComment(const VorbisComment &object): Prototype(object) { }
			inline VorbisComment(const ::FLAC__StreamMetadata &object): Prototype(object) { }
			inline VorbisComment(const ::FLAC__StreamMetadata *object): Prototype(object) { }
			//@}

			/** Constructs an object with copy control.  See
			 *  Prototype(::FLAC__StreamMetadata *object, bool copy).
			 */
			inline VorbisComment(::FLAC__StreamMetadata *object, bool copy): Prototype(object, copy) { }

			~VorbisComment();

			//@{
			/** Assign from another object.  Always performs a deep copy. */
			inline VorbisComment &operator=(const VorbisComment &object) { Prototype::operator=(object); return *this; }
			inline VorbisComment &operator=(const ::FLAC__StreamMetadata &object) { Prototype::operator=(object); return *this; }
			inline VorbisComment &operator=(const ::FLAC__StreamMetadata *object) { Prototype::operator=(object); return *this; }
			//@}

			/** Assigns an object with copy control.  See
			 *  Prototype::assign_object(::FLAC__StreamMetadata *object, bool copy).
			 */
			inline VorbisComment &assign(::FLAC__StreamMetadata *object, bool copy) { Prototype::assign_object(object, copy); return *this; }

			//@{
			/** Check for equality, performing a deep compare by following pointers. */
			inline bool operator==(const VorbisComment &object) const { return Prototype::operator==(object); }
			inline bool operator==(const ::FLAC__StreamMetadata &object) const { return Prototype::operator==(object); }
			inline bool operator==(const ::FLAC__StreamMetadata *object) const { return Prototype::operator==(object); }
			//@}

			//@{
			/** Check for inequality, performing a deep compare by following pointers. */
			inline bool operator!=(const VorbisComment &object) const { return Prototype::operator!=(object); }
			inline bool operator!=(const ::FLAC__StreamMetadata &object) const { return Prototype::operator!=(object); }
			inline bool operator!=(const ::FLAC__StreamMetadata *object) const { return Prototype::operator!=(object); }
			//@}

			unsigned get_num_comments() const;
			const FLAC__byte *get_vendor_string() const; // NUL-terminated UTF-8 string
			Entry get_comment(unsigned index) const;

			//! See FLAC__metadata_object_vorbiscomment_set_vendor_string()
			bool set_vendor_string(const FLAC__byte *string); // NUL-terminated UTF-8 string

			//! See FLAC__metadata_object_vorbiscomment_set_comment()
			bool set_comment(unsigned index, const Entry &entry);

			//! See FLAC__metadata_object_vorbiscomment_insert_comment()
			bool insert_comment(unsigned index, const Entry &entry);

			//! See FLAC__metadata_object_vorbiscomment_append_comment()
			bool append_comment(const Entry &entry);

			//! See FLAC__metadata_object_vorbiscomment_delete_comment()
			bool delete_comment(unsigned index);
		};

		/** CUESHEET metadata block.
		 *  See the \link flacpp_metadata_object overview \endlink for more,
		 *  and the <A HREF="../format.html#metadata_block_cuesheet">format specification</A>.
		 */
		class FLACPP_API CueSheet : public Prototype {
		public:
			/** Convenience class for encapsulating a cue sheet
			 *  track.
			 *
			 *  Always check is_valid() after the constructor or operator=
			 *  to make sure memory was properly allocated.
			 */
			class FLACPP_API Track {
			protected:
				::FLAC__StreamMetadata_CueSheet_Track *object_;
			public:
				Track();
				Track(const ::FLAC__StreamMetadata_CueSheet_Track *track);
				Track(const Track &track);
				Track &operator=(const Track &track);

				virtual ~Track();

				virtual bool is_valid() const; ///< Returns \c true iff object was properly constructed.


				inline FLAC__uint64 get_offset() const { return object_->offset; }
				inline FLAC__byte get_number() const { return object_->number; }
				inline const char *get_isrc() const { return object_->isrc; }
				inline unsigned get_type() const { return object_->type; }
				inline bool get_pre_emphasis() const { return object_->pre_emphasis; }

				inline FLAC__byte get_num_indices() const { return object_->num_indices; }
				::FLAC__StreamMetadata_CueSheet_Index get_index(unsigned i) const;

				inline const ::FLAC__StreamMetadata_CueSheet_Track *get_track() const { return object_; }

				inline void set_offset(FLAC__uint64 value) { object_->offset = value; }
				inline void set_number(FLAC__byte value) { object_->number = value; }
				void set_isrc(const char value[12]);
				void set_type(unsigned value);
				inline void set_pre_emphasis(bool value) { object_->pre_emphasis = value? 1 : 0; }

 				void set_index(unsigned i, const ::FLAC__StreamMetadata_CueSheet_Index &index);
				//@@@ It's awkward but to insert/delete index points
				//@@@ you must use the routines in the CueSheet class.
			};

			CueSheet();

			//@{
			/** Constructs a copy of the given object.  This form
			 *  always performs a deep copy.
			 */
			inline CueSheet(const CueSheet &object): Prototype(object) { }
			inline CueSheet(const ::FLAC__StreamMetadata &object): Prototype(object) { }
			inline CueSheet(const ::FLAC__StreamMetadata *object): Prototype(object) { }
			//@}

			/** Constructs an object with copy control.  See
			 *  Prototype(::FLAC__StreamMetadata *object, bool copy).
			 */
			inline CueSheet(::FLAC__StreamMetadata *object, bool copy): Prototype(object, copy) { }

			~CueSheet();

			//@{
			/** Assign from another object.  Always performs a deep copy. */
			inline CueSheet &operator=(const CueSheet &object) { Prototype::operator=(object); return *this; }
			inline CueSheet &operator=(const ::FLAC__StreamMetadata &object) { Prototype::operator=(object); return *this; }
			inline CueSheet &operator=(const ::FLAC__StreamMetadata *object) { Prototype::operator=(object); return *this; }
			//@}

			/** Assigns an object with copy control.  See
			 *  Prototype::assign_object(::FLAC__StreamMetadata *object, bool copy).
			 */
			inline CueSheet &assign(::FLAC__StreamMetadata *object, bool copy) { Prototype::assign_object(object, copy); return *this; }

			//@{
			/** Check for equality, performing a deep compare by following pointers. */
			inline bool operator==(const CueSheet &object) const { return Prototype::operator==(object); }
			inline bool operator==(const ::FLAC__StreamMetadata &object) const { return Prototype::operator==(object); }
			inline bool operator==(const ::FLAC__StreamMetadata *object) const { return Prototype::operator==(object); }
			//@}

			//@{
			/** Check for inequality, performing a deep compare by following pointers. */
			inline bool operator!=(const CueSheet &object) const { return Prototype::operator!=(object); }
			inline bool operator!=(const ::FLAC__StreamMetadata &object) const { return Prototype::operator!=(object); }
			inline bool operator!=(const ::FLAC__StreamMetadata *object) const { return Prototype::operator!=(object); }
			//@}

			const char *get_media_catalog_number() const;
			FLAC__uint64 get_lead_in() const;
			bool get_is_cd() const;

			unsigned get_num_tracks() const;
			Track get_track(unsigned i) const;

			void set_media_catalog_number(const char value[128]);
			void set_lead_in(FLAC__uint64 value);
			void set_is_cd(bool value);

			void set_index(unsigned track_num, unsigned index_num, const ::FLAC__StreamMetadata_CueSheet_Index &index);

			//! See FLAC__metadata_object_cuesheet_track_insert_index()
			bool insert_index(unsigned track_num, unsigned index_num, const ::FLAC__StreamMetadata_CueSheet_Index &index);

			//! See FLAC__metadata_object_cuesheet_track_delete_index()
			bool delete_index(unsigned track_num, unsigned index_num);

			//! See FLAC__metadata_object_cuesheet_set_track()
			bool set_track(unsigned i, const Track &track);

			//! See FLAC__metadata_object_cuesheet_insert_track()
			bool insert_track(unsigned i, const Track &track);

			//! See FLAC__metadata_object_cuesheet_delete_track()
			bool delete_track(unsigned i);

			//! See FLAC__metadata_object_cuesheet_is_legal()
			bool is_legal(bool check_cd_da_subset = false, const char **violation = 0) const;

			//! See FLAC__metadata_object_cuesheet_calculate_cddb_id()
			FLAC__uint32 calculate_cddb_id() const;
		};

		/** PICTURE metadata block.
		 *  See the \link flacpp_metadata_object overview \endlink for more,
		 *  and the <A HREF="../format.html#metadata_block_picture">format specification</A>.
		 */
		class FLACPP_API Picture : public Prototype {
		public:
			Picture();

			//@{
			/** Constructs a copy of the given object.  This form
			 *  always performs a deep copy.
			 */
			inline Picture(const Picture &object): Prototype(object) { }
			inline Picture(const ::FLAC__StreamMetadata &object): Prototype(object) { }
			inline Picture(const ::FLAC__StreamMetadata *object): Prototype(object) { }
			//@}

			/** Constructs an object with copy control.  See
			 *  Prototype(::FLAC__StreamMetadata *object, bool copy).
			 */
			inline Picture(::FLAC__StreamMetadata *object, bool copy): Prototype(object, copy) { }

			~Picture();

			//@{
			/** Assign from another object.  Always performs a deep copy. */
			inline Picture &operator=(const Picture &object) { Prototype::operator=(object); return *this; }
			inline Picture &operator=(const ::FLAC__StreamMetadata &object) { Prototype::operator=(object); return *this; }
			inline Picture &operator=(const ::FLAC__StreamMetadata *object) { Prototype::operator=(object); return *this; }
			//@}

			/** Assigns an object with copy control.  See
			 *  Prototype::assign_object(::FLAC__StreamMetadata *object, bool copy).
			 */
			inline Picture &assign(::FLAC__StreamMetadata *object, bool copy) { Prototype::assign_object(object, copy); return *this; }

			//@{
			/** Check for equality, performing a deep compare by following pointers. */
			inline bool operator==(const Picture &object) const { return Prototype::operator==(object); }
			inline bool operator==(const ::FLAC__StreamMetadata &object) const { return Prototype::operator==(object); }
			inline bool operator==(const ::FLAC__StreamMetadata *object) const { return Prototype::operator==(object); }
			//@}

			//@{
			/** Check for inequality, performing a deep compare by following pointers. */
			inline bool operator!=(const Picture &object) const { return Prototype::operator!=(object); }
			inline bool operator!=(const ::FLAC__StreamMetadata &object) const { return Prototype::operator!=(object); }
			inline bool operator!=(const ::FLAC__StreamMetadata *object) const { return Prototype::operator!=(object); }
			//@}

			::FLAC__StreamMetadata_Picture_Type get_type() const;
			const char *get_mime_type() const; // NUL-terminated printable ASCII string
			const FLAC__byte *get_description() const; // NUL-terminated UTF-8 string
			FLAC__uint32 get_width() const;
			FLAC__uint32 get_height() const;
			FLAC__uint32 get_depth() const;
			FLAC__uint32 get_colors() const; ///< a return value of \c 0 means true-color, i.e. 2^depth colors
			FLAC__uint32 get_data_length() const;
			const FLAC__byte *get_data() const;

			void set_type(::FLAC__StreamMetadata_Picture_Type type);

			//! See FLAC__metadata_object_picture_set_mime_type()
			bool set_mime_type(const char *string); // NUL-terminated printable ASCII string

			//! See FLAC__metadata_object_picture_set_description()
			bool set_description(const FLAC__byte *string); // NUL-terminated UTF-8 string

			void set_width(FLAC__uint32 value) const;
			void set_height(FLAC__uint32 value) const;
			void set_depth(FLAC__uint32 value) const;
			void set_colors(FLAC__uint32 value) const; ///< a value of \c 0 means true-color, i.e. 2^depth colors

			//! See FLAC__metadata_object_picture_set_data()
			bool set_data(const FLAC__byte *data, FLAC__uint32 data_length);
		};

		/** Opaque metadata block for storing unknown types.
		 *  This should not be used unless you know what you are doing;
		 *  it is currently used only internally to support forward
		 *  compatibility of metadata blocks.
		 *  See the \link flacpp_metadata_object overview \endlink for more,
		 */
		class FLACPP_API Unknown : public Prototype {
		public:
			Unknown();
			//
			//@{
			/** Constructs a copy of the given object.  This form
			 *  always performs a deep copy.
			 */
			inline Unknown(const Unknown &object): Prototype(object) { }
			inline Unknown(const ::FLAC__StreamMetadata &object): Prototype(object) { }
			inline Unknown(const ::FLAC__StreamMetadata *object): Prototype(object) { }
			//@}

			/** Constructs an object with copy control.  See
			 *  Prototype(::FLAC__StreamMetadata *object, bool copy).
			 */
			inline Unknown(::FLAC__StreamMetadata *object, bool copy): Prototype(object, copy) { }

			~Unknown();

			//@{
			/** Assign from another object.  Always performs a deep copy. */
			inline Unknown &operator=(const Unknown &object) { Prototype::operator=(object); return *this; }
			inline Unknown &operator=(const ::FLAC__StreamMetadata &object) { Prototype::operator=(object); return *this; }
			inline Unknown &operator=(const ::FLAC__StreamMetadata *object) { Prototype::operator=(object); return *this; }
			//@}

			/** Assigns an object with copy control.  See
			 *  Prototype::assign_object(::FLAC__StreamMetadata *object, bool copy).
			 */
			inline Unknown &assign(::FLAC__StreamMetadata *object, bool copy) { Prototype::assign_object(object, copy); return *this; }

			//@{
			/** Check for equality, performing a deep compare by following pointers. */
			inline bool operator==(const Unknown &object) const { return Prototype::operator==(object); }
			inline bool operator==(const ::FLAC__StreamMetadata &object) const { return Prototype::operator==(object); }
			inline bool operator==(const ::FLAC__StreamMetadata *object) const { return Prototype::operator==(object); }
			//@}

			//@{
			/** Check for inequality, performing a deep compare by following pointers. */
			inline bool operator!=(const Unknown &object) const { return Prototype::operator!=(object); }
			inline bool operator!=(const ::FLAC__StreamMetadata &object) const { return Prototype::operator!=(object); }
			inline bool operator!=(const ::FLAC__StreamMetadata *object) const { return Prototype::operator!=(object); }
			//@}

			const FLAC__byte *get_data() const;

			//! This form always copies \a data
			bool set_data(const FLAC__byte *data, unsigned length);
			bool set_data(FLAC__byte *data, unsigned length, bool copy);
		};

		/* \} */


		/** \defgroup flacpp_metadata_level0 FLAC++/metadata.h: metadata level 0 interface
		 *  \ingroup flacpp_metadata
		 *
		 *  \brief
		 *  Level 0 metadata iterators.
		 *
		 *  See the \link flac_metadata_level0 C layer equivalent \endlink
		 *  for more.
		 *
		 * \{
		 */

		FLACPP_API bool get_streaminfo(const char *filename, StreamInfo &streaminfo); ///< See FLAC__metadata_get_streaminfo().

		FLACPP_API bool get_tags(const char *filename, VorbisComment *&tags); ///< See FLAC__metadata_get_tags().
		FLACPP_API bool get_tags(const char *filename, VorbisComment &tags); ///< See FLAC__metadata_get_tags().

		FLACPP_API bool get_cuesheet(const char *filename, CueSheet *&cuesheet); ///< See FLAC__metadata_get_cuesheet().
		FLACPP_API bool get_cuesheet(const char *filename, CueSheet &cuesheet); ///< See FLAC__metadata_get_cuesheet().

		FLACPP_API bool get_picture(const char *filename, Picture *&picture, ::FLAC__StreamMetadata_Picture_Type type, const char *mime_type, const FLAC__byte *description, unsigned max_width, unsigned max_height, unsigned max_depth, unsigned max_colors); ///< See FLAC__metadata_get_picture().
		FLACPP_API bool get_picture(const char *filename, Picture &picture, ::FLAC__StreamMetadata_Picture_Type type, const char *mime_type, const FLAC__byte *description, unsigned max_width, unsigned max_height, unsigned max_depth, unsigned max_colors); ///< See FLAC__metadata_get_picture().

		/* \} */


		/** \defgroup flacpp_metadata_level1 FLAC++/metadata.h: metadata level 1 interface
		 *  \ingroup flacpp_metadata
		 *
		 *  \brief
		 *  Level 1 metadata iterator.
		 *
		 *  The flow through the iterator in the C++ layer is similar
		 *  to the C layer:
		 *    - Create a SimpleIterator instance
		 *    - Check SimpleIterator::is_valid()
		 *    - Call SimpleIterator::init() and check the return
		 *    - Traverse and/or edit.  Edits are written to file
		 *      immediately.
		 *    - Destroy the SimpleIterator instance
		 *
		 *  The ownership of pointers in the C++ layer follows that in
		 *  the C layer, i.e.
		 *    - The objects returned by get_block() are yours to
		 *      modify, but changes are not reflected in the FLAC file
		 *      until you call set_block().  The objects are also
		 *      yours to delete; they are not automatically deleted
		 *      when passed to set_block() or insert_block_after().
		 *
		 *  See the \link flac_metadata_level1 C layer equivalent \endlink
		 *  for more.
		 *
		 * \{
		 */

		/** This class is a wrapper around the FLAC__metadata_simple_iterator
		 *  structures and methods; see the
		 * \link flacpp_metadata_level1 usage guide \endlink and
		 * ::FLAC__Metadata_SimpleIterator.
		 */
		class FLACPP_API SimpleIterator {
		public:
			/** This class is a wrapper around FLAC__Metadata_SimpleIteratorStatus.
			 */
			class FLACPP_API Status {
			public:
				inline Status(::FLAC__Metadata_SimpleIteratorStatus status): status_(status) { }
				inline operator ::FLAC__Metadata_SimpleIteratorStatus() const { return status_; }
				inline const char *as_cstring() const { return ::FLAC__Metadata_SimpleIteratorStatusString[status_]; }
			protected:
				::FLAC__Metadata_SimpleIteratorStatus status_;
			};

			SimpleIterator();
			virtual ~SimpleIterator();

			bool is_valid() const; ///< Returns \c true iff object was properly constructed.

			bool init(const char *filename, bool read_only, bool preserve_file_stats); ///< See FLAC__metadata_simple_iterator_init().

			Status status();                                                    ///< See FLAC__metadata_simple_iterator_status().
			bool is_writable() const;                                           ///< See FLAC__metadata_simple_iterator_is_writable().

			bool next();                                                        ///< See FLAC__metadata_simple_iterator_next().
			bool prev();                                                        ///< See FLAC__metadata_simple_iterator_prev().
			bool is_last() const;                                               ///< See FLAC__metadata_simple_iterator_is_last().

			off_t get_block_offset() const;                                     ///< See FLAC__metadata_simple_iterator_get_block_offset().
			::FLAC__MetadataType get_block_type() const;                        ///< See FLAC__metadata_simple_iterator_get_block_type().
			unsigned get_block_length() const;                                  ///< See FLAC__metadata_simple_iterator_get_block_length().
			bool get_application_id(FLAC__byte *id);                            ///< See FLAC__metadata_simple_iterator_get_application_id().
			Prototype *get_block();                                             ///< See FLAC__metadata_simple_iterator_get_block().
			bool set_block(Prototype *block, bool use_padding = true);          ///< See FLAC__metadata_simple_iterator_set_block().
			bool insert_block_after(Prototype *block, bool use_padding = true); ///< See FLAC__metadata_simple_iterator_insert_block_after().
			bool delete_block(bool use_padding = true);                         ///< See FLAC__metadata_simple_iterator_delete_block().

		protected:
			::FLAC__Metadata_SimpleIterator *iterator_;
			void clear();
		};

		/* \} */


		/** \defgroup flacpp_metadata_level2 FLAC++/metadata.h: metadata level 2 interface
		 *  \ingroup flacpp_metadata
		 *
		 *  \brief
		 *  Level 2 metadata iterator.
		 *
		 *  The flow through the iterator in the C++ layer is similar
		 *  to the C layer:
		 *    - Create a Chain instance
		 *    - Check Chain::is_valid()
		 *    - Call Chain::read() and check the return
		 *    - Traverse and/or edit with an Iterator or with
		 *      Chain::merge_padding() or Chain::sort_padding()
		 *    - Write changes back to FLAC file with Chain::write()
		 *    - Destroy the Chain instance
		 *
		 *  The ownership of pointers in the C++ layer is slightly
		 *  different than in the C layer, i.e.
		 *    - The objects returned by Iterator::get_block() are NOT
		 *      owned by the iterator and should be deleted by the
		 *      caller when finished, BUT, when you modify the block,
		 *      it will directly edit what's in the chain and you do
		 *      not need to call Iterator::set_block().  However the
		 *      changes will not be reflected in the FLAC file until
		 *      the chain is written with Chain::write().
		 *    - When you pass an object to Iterator::set_block(),
		 *      Iterator::insert_block_before(), or
		 *      Iterator::insert_block_after(), the iterator takes
		 *      ownership of the block and it will be deleted by the
		 *      chain.
		 *
		 *  See the \link flac_metadata_level2 C layer equivalent \endlink
		 *  for more.
		 *
		 * \{
		 */

		/** This class is a wrapper around the FLAC__metadata_chain
		 *  structures and methods; see the
		 * \link flacpp_metadata_level2 usage guide \endlink and
		 * ::FLAC__Metadata_Chain.
		 */
		class FLACPP_API Chain {
		public:
			/** This class is a wrapper around FLAC__Metadata_ChainStatus.
			 */
			class FLACPP_API Status {
			public:
				inline Status(::FLAC__Metadata_ChainStatus status): status_(status) { }
				inline operator ::FLAC__Metadata_ChainStatus() const { return status_; }
				inline const char *as_cstring() const { return ::FLAC__Metadata_ChainStatusString[status_]; }
			protected:
				::FLAC__Metadata_ChainStatus status_;
			};

			Chain();
			virtual ~Chain();

			friend class Iterator;

			bool is_valid() const; ///< Returns \c true iff object was properly constructed.

			Status status();                                                ///< See FLAC__metadata_chain_status().

			bool read(const char *filename, bool is_ogg = false);                                ///< See FLAC__metadata_chain_read(), FLAC__metadata_chain_read_ogg().
			bool read(FLAC__IOHandle handle, FLAC__IOCallbacks callbacks, bool is_ogg = false);  ///< See FLAC__metadata_chain_read_with_callbacks(), FLAC__metadata_chain_read_ogg_with_callbacks().

			bool check_if_tempfile_needed(bool use_padding);                ///< See FLAC__metadata_chain_check_if_tempfile_needed().

			bool write(bool use_padding = true, bool preserve_file_stats = false); ///< See FLAC__metadata_chain_write().
			bool write(bool use_padding, ::FLAC__IOHandle handle, ::FLAC__IOCallbacks callbacks); ///< See FLAC__metadata_chain_write_with_callbacks().
			bool write(bool use_padding, ::FLAC__IOHandle handle, ::FLAC__IOCallbacks callbacks, ::FLAC__IOHandle temp_handle, ::FLAC__IOCallbacks temp_callbacks); ///< See FLAC__metadata_chain_write_with_callbacks_and_tempfile().

			void merge_padding();                                           ///< See FLAC__metadata_chain_merge_padding().
			void sort_padding();                                            ///< See FLAC__metadata_chain_sort_padding().

		protected:
			::FLAC__Metadata_Chain *chain_;
			virtual void clear();
		};

		/** This class is a wrapper around the FLAC__metadata_iterator
		 *  structures and methods; see the
		 * \link flacpp_metadata_level2 usage guide \endlink and
		 * ::FLAC__Metadata_Iterator.
		 */
		class FLACPP_API Iterator {
		public:
			Iterator();
			virtual ~Iterator();

			bool is_valid() const; ///< Returns \c true iff object was properly constructed.


			void init(Chain &chain);                       ///< See FLAC__metadata_iterator_init().

			bool next();                                   ///< See FLAC__metadata_iterator_next().
			bool prev();                                   ///< See FLAC__metadata_iterator_prev().

			::FLAC__MetadataType get_block_type() const;   ///< See FLAC__metadata_iterator_get_block_type().
			Prototype *get_block();                        ///< See FLAC__metadata_iterator_get_block().
			bool set_block(Prototype *block);              ///< See FLAC__metadata_iterator_set_block().
			bool delete_block(bool replace_with_padding);  ///< See FLAC__metadata_iterator_delete_block().
			bool insert_block_before(Prototype *block);    ///< See FLAC__metadata_iterator_insert_block_before().
			bool insert_block_after(Prototype *block);     ///< See FLAC__metadata_iterator_insert_block_after().

		protected:
			::FLAC__Metadata_Iterator *iterator_;
			virtual void clear();
		};

		/* \} */

	}
}

#endif
