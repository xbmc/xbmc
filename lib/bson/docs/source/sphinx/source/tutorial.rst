MongoDB C Driver Tutorial
=========================

This document shows how to use MongoDB from C. If you're not familiar with MongoDB.
you'll want to get a brief overview of the database and its shell API. The official
tutorial is a great place to start.

Next, you'll want to install and run MongoDB.

A working C program complete with examples from this tutorial can be
found in the examples folder of the source distribution.

C API
-----

When writing programs with the C driver, you'll be using four different
entities: connections, cursors, bson objects, and bson iterators. The APIs
for each of these follow a similiar pattern. You start by allocating an object,
either on the stack or the heap (the examples that follow all use the stack). You then
call an ``init`` function and use other function to build the object. When you're finished,
you pass the object to a ``destroy`` function.

So, for instance, to create a new connection, start by allocating a ``mongo`` object:

.. code-block:: c

    mongo conn;

Next, initialize it:

.. code-block:: c

    mongo_init( &conn );

Set any optional values, like a timeout, and then call ``mongo_connect``:

.. code-block:: c

    mongo_set_op_timeout( &conn, 1000 );
    mongo_connect( &conn, "127.0.0.1", 27017 );

When you're finished, destroy the mongo object:

.. code-block:: c

    mongo_destroy( &conn );

There are more details, but that's the basic pattern. Keep this in mind
as you learn the API and start using the driver.

Connecting
----------

Let's start by that connects to the database:

.. code-block:: c

    #include <stdio.h>
    #include "mongo.h"

    int main() {
      mongo conn[1];
      int status = mongo_connect( conn, "127.0.0.1", 27017 );

      if( status != MONGO_OK ) {
          switch ( conn->err ) {
            case MONGO_CONN_NO_SOCKET:  printf( "no socket\n" ); return 1;
            case MONGO_CONN_FAIL:       printf( "connection failed\n" ); return 1;
            case MONGO_CONN_NOT_MASTER: printf( "not master\n" ); return 1;
          }
      }

      mongo_destroy( conn );

      return 0;
    }

Building the sample program
---------------------------

If you are using ``gcc`` on Linux or OS X, you can compile with something like this,
depending on location of your include files:

.. code-block:: bash

    $ gcc -Isrc --std=c99 /path/to/mongo-c-driver/src/*.c -I /path/to/mongo-c-driver/src/ tutorial.c -o tutorial
    $ ./tutorial
    connection succeeded
    connection closed


Connecting to a replica set
---------------------------

The API for connecting to a replica set is slightly different. First you initialize
the connection object, specifying the replica set's name (in this case, "shard1"),
then you add seed nodes, and finally you connect. Here's an example:

.. code-block:: c

    #include "mongo.h"

    int main() {
      mongo conn[1];

      mongo_replset_init( conn, "shard1" );
      mongo_replset_add_seed( conn, "10.4.3.22", 27017 );
      mongo_replset_add_seed( conn, "10.4.3.32", 27017 );

      status = mongo_replset_connect( conn );

      if( status != MONGO_OK ) {
          /* Check conn->err for error code. */
      }

      mongo_destroy( conn );

      return 0;
    }

BSON
----

MongoDB database stores data in a format called *BSON*. BSON is a JSON-like binary object format.
To create BSON objects


.. code-block:: c

  bson b[1];

  bson_init( b )
  bson_append_string( b, "name", "Joe" );
  bson_append_int( b, "age", 33 );
  bson_finish( b );

  mongo_insert( conn, b );

  bson_destroy( b );

Use the ``bson_append_new_oid()`` function to add an object id to your object.
The server will add an object id to the ``_id`` field if it is not included explicitly,
but it's best to create it client-side. When you do create the id, be sure to place it
at the beginning of the object, as we do here:

.. code-block:: c

    bson b[1];

    bson_init( b );
    bson_append_new_oid( b, "_id" );
    bson_append_string( b, "name", "Joe" );
    bson_append_int( b, "age", 33 );
    bson_finish( b );

When you're done using the ``bson`` object, remember pass it to
``bson_destroy()`` to free up the memory allocated by the buffer.

.. code-block:: c

    bson_destroy( b );

Inserting a single document
---------------------------

Here's how we save our person object to the database's "people" collection:

.. code-block:: c

    mongo_insert( conn, "tutorial.people", b );

The first parameter to ``mongo_insert`` is the pointer to the ``mongo_connection``
object. The second parameter is the namespace, which include the database name, followed
by a dot followed by the collection name. Thus, ``tutorial`` is the database and ``people``
is the collection name. The third parameter is a pointer to the ``bson`` object that
we created before.

Inserting a batch of documents
------------------------------

We can do batch inserts as well:

.. code-block:: c

    static void tutorial_insert_batch( mongo_connection *conn ) {
      bson *p, **ps;
      char *names[4];
      int ages[] = { 29, 24, 24, 32 };
      int i, n = 4;
      names[0] = "Eliot"; names[1] = "Mike"; names[2] = "Mathias"; names[3] = "Richard";

      ps = ( bson ** )malloc( sizeof( bson * ) * n);

      for ( i = 0; i < n; i++ ) {
        p = ( bson * )malloc( sizeof( bson ) );
        bson_init( p );
        bson_append_new_oid( p_buf, "_id" );
        bson_append_string( p_buf, "name", names[i] );
        bson_append_int( p_buf, "age", ages[i] );
        bson_finish( p );
        ps[i] = p;
      }

      mongo_insert_batch( conn, "tutorial.persons", ps, n );

      for ( i = 0; i < n; i++ ) {
        bson_destroy( ps[i] );
        free( ps[i] );
      }
    }

Simple Queries
--------------

Let's now fetch all objects from the ``persons`` collection, and display them.

.. code-block:: c

    static void tutorial_empty_query( mongo_connection *conn) {
      mongo_cursor cursor[1];
      mongo_cursor_init( cursor, conn, "tutorial.persons" );

      while( mongo_cursor_next( cursor ) == MONGO_OK )
        bson_print( &cursor->current );

      mongo_cursor_destroy( cursor );
    }

Here we use the most basic possible cursor, which iterates over all documents. This is the
equivalent of running ``db.persons.find()`` from the shell.

You initialize a cursor with ``mongo_cursor_init()``. Whenever you finish with a cursor,
you must pass it to ``mongo_cursor_destroy()``.

We use ``bson_print()`` to print an abbreviated JSON string representation of the object.

Let's now write a function which prints out the name of all persons
whose age is 24:

.. code-block:: c

    static void tutorial_simple_query( mongo_connection *conn ) {
      bson query[1];
      mongo_cursor cursor[1];

      bson_init( query );
      bson_append_int( query_buf, "age", 24 );
      bson_finish( query );

      mongo_cursor_init( cursor, conn, "tutorial.persons" );
      mongo_cursor_set_query( cursor, query );

      while( mongo_cursor_next( cursor ) == MONGO_OK ) {
        bson_iterator iterator[1];
        if ( bson_find( iterator, mongo_cursor_bson( cursor ), "name" )) {
            printf( "name: %s\n", bson_iterator_string( iterator ) );
        }
      }

      bson_destroy( query );
      mongo_cursor_destroy( cursor );
    }

Our query above, written as JSON, is equivalent to the following from the JavaScript shell:

.. code-block:: javascript

    use tutorial
    db.persons.find( { age: 24 } )

Complex Queries
---------------

Sometimes we want to do more then a simple query. We may want the results to
be sorted in a special way, or what the query to use a certain index.

Let's add a sort clause to our previous query. This requires some knowledge of the
implementation of query specs in MongoDB. A query spec can either consist of:

1. A query matcher alone, as in our previous example.

or

2. A query matcher, sort clause, hint enforcer, or explain directive. Each of these
   is wrapped by the keys ``$query``, ``$orderby``, ``$hint``, and ``$explain``, respectively.
   Most of the time, you'll only use ``$query`` and ``$orderby``.

To add a sort clause to our previous query, we change our query spec from this:

.. code-block:: c

    bson_init( query );
    bson_append_int( query, "age", 24 );
    bson_finish( query );

to this:

.. code-block:: c

    bson_init( query );
      bson_append_start_object( query, "$query" );
        bson_append_int( query, "age", 24 );
      bson_append_finish_object( query );

      bson_append_start_object( query, "$orderby" );
        bson_append_int( query, "name", 1);
      bson_append_finish_object( query );
    bson_finish( query );

This is equivalent to the following query from the MongoDB shell:

.. code-block:: javascript

    db.persons.find( { age: 24 } ).sort( { name: 1 } );


Updating documents
------------------

Use the ``mongo_update()`` function to perform updates.
For example the following update in the MongoDB shell:

.. code-block:: javascript

    use tutorial
    db.persons.update( { name : 'Joe', age : 33 },
                       { $inc : { visits : 1 } } )

is equivalent to the following C function:

.. code-block:: c

    static void tutorial_update( mongo_connection *conn ) {
      bson cond[1], op[1];

      bson_init( cond );
        bson_append_string( cond, "name", "Joe");
        bson_append_int( cond, "age", 33);
      bson_finish( cond );

      bson_init( op );
        bson_append_start_object( op, "$inc" );
          bson_append_int( op, "visits", 1 );
        bson_append_finish_object( op );
      bson_finish( op );

      mongo_update( conn, "tutorial.persons", cond, op, MONGO_UPDATE_BASIC );

      bson_destroy( cond );
      bson_destroy( op );
    }

The final argument to ``mongo_update()`` is a bitfield storing update options. If
you want to update all documents matching the ``cond``, you must use ``MONGO_UPDATE_MULTI``.
For upserts, use ``MONGO_UPDATE_UPSERT``. Here's an example:

.. code-block:: c

      mongo_update( conn, "tutorial.persons", cond, op, MONGO_UPDATE_MULTI );

Indexing
--------

Now we'll create a couple of indexes. The first is a simple index on ``name``, and
the second is a compound index on ``name`` and ``age``.

.. code-block:: c

    static void tutorial_index( mongo_connection *conn ) {
      bson key[1];

      bson_init( key );
      bson_append_int( key, "name", 1 );
      bson_finish( key );

      mongo_create_index( conn, "tutorial.persons", key, 0, NULL );

      bson_destroy( key );

      printf( "simple index created on \"name\"\n" );

      bson_init( key );
      bson_append_int( key, "age", 1 );
      bson_append_int( key, "name", 1 );
      bson_finish( key );

      mongo_create_index( conn, "tutorial.persons", key, 0, NULL );

      bson_destroy( key );

      printf( "compound index created on \"age\", \"name\"\n" );
    }



Further Reading
---------------

This overview just touches on the basics of using Mongo from C. For more examples,
check out the other documentation pages, and have a look at the driver's test cases.
