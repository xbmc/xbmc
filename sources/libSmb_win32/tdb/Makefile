#
# Makefile for tdb directory
#

CFLAGS = -DSTANDALONE -DTDB_DEBUG -g -DHAVE_MMAP=1
CC = gcc

ADMINPROGS = tdbdump tdbbackup
PROGS = tdbtest tdbtool tdbtorture
TDB_OBJ = tdb.o spinlock.o tdbback.o

default: $(PROGS)

admintools: $(ADMINPROGS)

tdbtest: tdbtest.o $(TDB_OBJ)
	$(CC) $(CFLAGS) -o tdbtest tdbtest.o $(TDB_OBJ) -lgdbm

tdbtool: tdbtool.o $(TDB_OBJ)
	$(CC) $(CFLAGS) -o tdbtool tdbtool.o $(TDB_OBJ)

tdbtorture: tdbtorture.o $(TDB_OBJ)
	$(CC) $(CFLAGS) -o tdbtorture tdbtorture.o $(TDB_OBJ)

tdbdump: tdbdump.o $(TDB_OBJ)
	$(CC) $(CFLAGS) -o tdbdump tdbdump.o $(TDB_OBJ)

tdbbackup: tdbbackup.o $(TDB_OBJ)
	$(CC) $(CFLAGS) -o tdbbackup tdbbackup.o $(TDB_OBJ)

clean:
	rm -f $(PROGS) *.o *~ *% core test.db test.tdb test.gdbm
