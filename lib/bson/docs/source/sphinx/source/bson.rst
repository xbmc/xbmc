BSON
=============================

BSON (i.e., binary structured object notation) is the binary format used
by MongoDB to store data and express queries and commands. To work with
MongoDB is to trade in BSON objects. This document describes how to
create, read, and destroy BSON objects using the MongoDB C Driver.

Libraries
---------

A brief note on libraries.

When you compile the driver, the BSON library is included in the
driver. This means that when you include ``mongo.h``, you have access
to all the functions declared in ``bson.h``.

If you want to use BSON independently, you don't need ``libmongoc``: when you compile
the driver, you'll also get shared and static libraries for ``libbson``. You
can link to this library and simple require ``bson.h``.

Using BSON objects
------------------

The pattern of BSON object usage is pretty simple. Here are the steps:

1. Initiate a new BSON object.
2. Construct the object using the bson_append_* methods.
3. Pass the object to bson_finish() to finalize it. The object is now ready to use.
4. When you're done with it, pass the object to bson_destroy() to free up any allocated
   memory.

To demonstrate, let's create a BSON object corresponding to the simple JSON object
``{count: 1001}``.

.. code-block:: c

    bson b[1];

    bson_init( b );
    bson_append_int( b, "count", 1001 );
    bson_finish( b );

    /* BSON object now ready for use */

    bson_destroy( b );

That's all there is to creating a basic object.

Creating complex BSON objects
_____________________________

BSON objects can contain arrays as well as sub-objects. Here
we'll see how to create these by building the bson object
corresponding to the following JSON object:

.. code-block:: javascript

    {
      name: "Kyle",

      colors: [ "red", "blue", "green" ],

      address: {
        city: "New York",
        zip: "10011-4567"
      }
    }

.. code-block:: c

     bson b[1];

     bson_init( b );
     bson_append_string( b, "name", "Kyle" );

     bson_append_start_array( b, "colors" );
       bson_append_string( b, "0", "red" );
       bson_append_string( b, "1", "blue" );
       bson_append_string( b, "2", "green" );
     bson_append_finish_array( b );

     bson_append_start_object( b, "address" );
       bson_append_string( b, "city", "New York" );
       bson_append_string( b, "zip", "10011-4567" );
     bson_append_finish_object( b );

     if( bson_finish( b ) != BSON_OK )
         printf(" Error. ");

Notice that for the array, we have to manually set the index values
from "0" to *n*, where *n* is the number of elements in the array.

You'll notice that some knowledge of the BSON specification and
of the available types is necessary. For that, take a few minutes to
consult the `official BSON specification <http://bsonspec.org>`_.

Error handling
--------------

The names of BSON object values, as well as all strings, must be
encoded as valid UTF-8. The BSON library will automatically check
the encoding of strings as you create BSON objects, and if the objects
are invalid, you'll be able to check for this condition. All of the
bson_append_* methods will return either BSON_OK for BSON_ERROR. You
can check in your code for the BSON_ERROR condition and then see the
exact nature of the error by examining the bson->err field. This bitfield
can contain any of the following values:

* BSON_VALID
* BSON_NOT_UTF8
* BSON_FIELD_HAS_DOT
* BSON_FIELD_INIT_DOLLAR
* BSON_ALREADY_FINISHED

The most important of these is ``BSON_NOT_UTF8`` because the BSON
objects cannot be used with MongoDB if they're not valid UTF8.

To keep your code clean, you may want to check for BSON_OK only when
calling ``bson_finish()``. If the object is not valid, it will not be
finished, so it's quite important to check the return code here.

Reading BSON objects
--------------------

You can read through a BSON object using a ``bson_iterator``. For
a complete example, you may want to read through the implementation
of ``bson_print_raw()`` (in ``bson.h``). But the basic idea is to
initialize a ``bson_iterator`` object and then iterate over each
successive element using ``bson_iterator_next()``. Let's take an
example. Suppose we have a finished object of type ``bson*`` called ``b``:

.. code-block:: c


   bson_iterator i[1];
   bson_type type;
   const char * key;

   bson_iterator_init( i, b );

   type = bson_iterator_next( i );
   key = bson_iterator_key( i );

   printf( "Type: %d, Key: %s\n", type, key );

We've advanced to the first element in the object, and we can print
both it's BSON numeric type and its key name. To print the value,
we need to use the type to find the correct method for reading the
value. For instance, if the element is a string, then we use
``bson_iterator_string`` to return the result:

.. code-block:: c

   printf( "Value: %s\n", bson_iterator_string( i ) );

In addition to iterating over each successive BSON element,
we can use the ``bson_find()`` function to jump directly
to an element by name. Again, suppose that ``b`` is a pointer
to a ``bson`` object. If we want to jump to the element
named "address", we use ``bson_find()`` like so:

.. code-block:: c

   bson_iterator i[1], sub[i];
   bson_type type;

   type = bson_find( i, b, "address" );

This will initialize the iterator, ``i``, and position
it at the element named "address". The return value
will be the "address" element's type.

Reading sub-objects and arrays
------------------------------

Since "address" is a sub-object, we need to specially
iterate it. To do that, we get the raw value and initialize
a new BSON iterator like so:

.. code-block:: c

   type = bson_find( i, b, "address" );

   bson_iterator_subiterator( i, sub );

The function ``bson_iterator_subiterator`` initializes
the iterator ``sub`` and points it to the beginning of the
sub-object. From there, we can iterate over
``sub`` until we reach ``BSON_EOO``, indicating the end of the
sub-object.

If you want to work with a sub-object by itself, there's
a function, ``bson_iterator_subobject``, for initializing
a new ``bson`` object with the value of the sub-object. Note
that this does not copy the object. If you want a copy of the
object, use ``bsop_copy()``.

.. code-block:: c

   bson copy[1];

   bson_copy( copy, sub );

Getting a Raw BSON Pointer
--------------------------

Sometimes you'll want to access the ``char *`` that
points to the buffer storing the raw BSON object. For that,
use the ``bson_data()`` function. You can use this in concert
with the bson_iterator_from_buffer() function to initialize an
iterator:

.. code-block:: c

   bson_iterator i[1];

   bson_iterator_from_buffer( i, bson_data( b ) );
