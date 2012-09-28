Some notes on how to make this easier to merge upstream.

* Try to minimize the changes to files under xbmc/*
* If you have to change XBMC files, please consider the following:
	* Mark your changes with comments like /* PLEX */ mychanges /* END PLEX */
	* Commit in very small batches. If you add a function to a XBMC class, commit that in a separate commit, make it much easier to rebase
	* Where applicable, name the new methods to something prefixed by Plex
	* If you have to make a huge change to the file, copy it to the Plex directory and leave the original in the xbmc directory.
	


=== Building ===
The only thing you would need is CMake 2.8.8 or newer. 

Don't forget to update submodules.

The enter the directory plex/build and run ./cmake-debug.sh and it will create the Makefiles.
Now you can run make -jX to get a binary and then make install to create a Plex.app in output.
