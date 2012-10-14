Connections
=============================

All operations against a MongoDB server or cluster require a connection object. This document
describes how to create and manage these connections.

Basic connections
-----------------

Use a basic connection to connect to a single MongoDB instances (``mongod``) or
to the router for a shard cluster (``mongos``).

.. code-block:: c

   mongo conn[1];
   int result;

   result = mongo_connect( conn, "127.0.0.1", 27017 );

First we create the ``mongo`` object to manage the connection. Then we connect
using ``mongo_connect``. If the function returns ``MONGO_OK``, then we've
successfully connected.

Notice that when specifying the host, we must use dot-decimal notation. If you'd like
to use a hostname, then you'll have to compile the driver with the ``--use-platform=LINUX``
option and ensure that ``_MONGO_USE_GETADDRINFO`` is defined.

The C driver now also supports connecting to mongodb through unix domain 
sockets (only on POSIX systems, of course). To connect to a unix domain socket, 
pass the path to the socket in place of the host address to ``mongo_connect`` 
and pass a negative number in as the port number. For instance, 

.. code-block:: c

    result = mongo_connect( conn, "/tmp/mongodb-27017.sock", -1 );

In the event of an error, the result will be ``MONGO_ERROR``. You can then check the error
value by examining the connection's ``err`` field. Continuing:

.. code-block:: c

   if( result != MONGO_OK ) {
     switch( conn->err ) {
       case MONGO_CONN_NO_SOCKET: break;  /**< Could not create a socket. */
       case MONGO_CONN_FAIL: break;       /**< An error occured while calling connect(). */
       case MONGO_CONN_ADDR_FAIL: break;  /**< An error occured while calling getaddrinfo(). */
       case MONGO_CONN_NOT_MASTER: break; /**< Warning: connected to a non-master node (read-only). */
   }

These are the most likely error scenarios. For all possible errors,
see the enum ``mongo_error_t``, and reference all constants beginning
with ``MONGO_CONN``.

Once you've finished with your connection object, be sure to pass it to
``mongo_destroy()``. This will close the socket and clean up any allocated
memory:

.. code-block:: c

   mongo_destroy( conn );

Replica set connections
-----------------------

Use a replica set connection to connect to a replica set.

The problem with connecting to a replica set is that you don't necessarily
know which node is the primary node at connection time. This MongoDB C driver
automatically figures out which node is the primary and then connects to it.

To connection, you must provide:

* The replica set's name

And

* At least one seed node.

Here's how you go about that:

.. code-block:: c

   mongo conn[1];
   int result;

   mongo_replset_init( conn, "rs-dc-1" );
   mongo_replset_add_seed( conn, '10.4.3.1', 27017 );
   mongo_replset_add_seed( conn, '10.4.3.2', 27017 );

   result = mongo_replset_connect( conn );

First we initiaize the connection object, providing the name of the replica set,
in this case, "rs-dc-1." Next, we add two seed nodes. Finally, we connect
by pass the connection to ``mongo_replset_connect``.

As with the basic connection, we'll want to check for any errors on connect. Notice
that there are two more error conditions we check for:

.. code-block:: c

   if( result != MONGO_OK ) {
     switch( conn->err )
       MONGO_CONN_NO_SOCKET: break;    /**< Could not create a socket. */
       MONGO_CONN_FAIL: break;         /**< An error occured while calling connect(). */
       MONGO_CONN_ADDR_FAIL: break;    /**< An error occured while calling getaddrinfo(). */
       MONGO_CONN_NOT_MASTER: break;   /**< Warning: connected to a non-master node (read-only). */
       MONGO_CONN_BAD_SET_NAME: break; /**< Given rs name doesn't match this replica set. */
       MONGO_CONN_NO_PRIMARY: break;   /**< Can't find primary in replica set. Connection closed. */
   }

When finished, be sure to destroy the connection object:

.. code-block:: c

   mongo_destroy( conn );

Timeouts
--------

You can set a timeout for read and write operation on the connection at any time:

.. code-block:: c

   mongo_set_op_timeout( conn, 1000 );

This will set a 1000ms read-write timeout on the socket. If an operation fails,
you'll see a generic MONGO_IO_ERROR on the connection's ``err`` field. Future
versions of this driver will provide a more granular error code.

Note this this will work only if you've compiled with driver with timeout support.

I/O Errors and Reconnecting
--------------------------

As you begin to use connection object to read and write data from MongoDB,
you may ocassionally encounter a ``MONGO_IO_ERROR``. In most cases,
you'll want to reconnect when you see this. Here's a very basic example:

.. code-block:: c

   bson b[1];

   bson_init( b );
   bson_append_string( b, "hello", "world" );
   bson_finish( b );

   if( mongo_insert( conn, b ) == MONGO_ERROR && conn->err == MONGO_IO_ERROR )
       mongo_reconnect( conn );

When reconnecting, you'll want to check the return value to ensure that the connection
has succeeded. If you ever have any doubts about whether you're really connection,
you can verify the health of the connection like so:

.. code-block:: c

   mongo_check_connection( conn );

This function will return ``MONGO_OK`` if we're in fact connected.
