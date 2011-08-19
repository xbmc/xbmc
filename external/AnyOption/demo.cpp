/*
 * Here is a sample of how to use AnyOption to
 * parse comand line argumnets and an ptions file
 *
 * Create  sample.txt as follows
 *
 *      # sample options file
 *      # this is a comment 
 *      zip  
 *      size  : 42 
 *      title : This is a test title.
 *
 * Run the sample with any combination of the options
 *
 *      a.out -c --zip -s 20 --name foo.jpg argtest1 argtest2 
 */

#include "anyoption.h"

void example( int argc, char* argv[] );

int
main( int argc, char* argv[] )
{
        example( argc, argv ); 
        return 0 ;
}

void
example( int argc, char* argv[] )
{

        /* 1. CREATE AN OBJECT */
        AnyOption *opt = new AnyOption();

        /* 2. SET PREFERENCES  */
        //opt->noPOSIX(); /* do not check for POSIX style character options */
        //opt->setVerbose(); /* print warnings about unknown options */
        //opt->autoUsagePrint(true); /* print usage for bad options */

        /* 3. SET THE USAGE/HELP   */
        opt->addUsage( "" );
        opt->addUsage( "Usage: " );
        opt->addUsage( "" );
        opt->addUsage( " -h  --help  		Prints this help " );
        opt->addUsage( " -s  --size 42 	        Image Size " );
        opt->addUsage( " -z  --zip  		Compress Image " );
        opt->addUsage( " -c   			convert Image " );
        opt->addUsage( "     --name image.jpg	Image Name " );
        opt->addUsage( "" );

        /* 4. SET THE OPTION STRINGS/CHARACTERS */

	/* by default all  options  will be checked on the command line and from option/resource file */
        opt->setFlag(  "help", 'h' );   /* a flag (takes no argument), supporting long and short form */ 
        opt->setOption(  "size", 's' ); /* an option (takes an argument), supporting long and short form */
        opt->setOption(  "name" );      /* an option (takes an argument), supporting only long form */
        opt->setFlag( 'c' );            /* a flag (takes no argument), supporting only short form */

	/* for options that will be checked only on the command and line not in option/resource file */
        opt->setCommandFlag(  "zip" , 'z'); /* a flag (takes no argument), supporting long and short form */

	/* for options that will be checked only from the option/resource file */
        opt->setFileOption(  "title" ); /* an option (takes an argument), supporting only long form */

        /* 5. PROCESS THE COMMANDLINE AND RESOURCE FILE */

	/* read options from a  option/resource file with ':' separated opttions or flags, one per line */
        opt->processFile( "/home/user/.options" );  
	/* go through the command line and get the options  */
        opt->processCommandArgs( argc, argv );

	if( ! opt->hasOptions()) { /* print usage if no options */
                opt->printUsage();
	        delete opt;
		return;
	}

        /* 6. GET THE VALUES */
        if( opt->getFlag( "help" ) || opt->getFlag( 'h' ) ) 
                opt->printUsage();
	if( opt->getValue( 's' ) != NULL  || opt->getValue( "size" ) != NULL  )
        	cout << "size = " << opt->getValue( 's' ) << endl ;
	if( opt->getValue( "name" ) != NULL )
       	 	cout << "name = " << opt->getValue( "name" ) << endl ;
	if( opt->getValue( "title" ) != NULL )
        	cout << "title = " << opt->getValue( "title" ) << endl ;
        if( opt->getFlag( 'c' ) )  
		cout << "c = flag set " << endl ;
        if( opt->getFlag( 'z' ) || opt->getFlag( "zip" ) )  
		cout << "zip = flag set " << endl ;
        cout << endl ;

	/* 7. GET THE ACTUAL ARGUMENTS AFTER THE OPTIONS */
	for( int i = 0 ; i < opt->getArgc() ; i++ ){
		cout << "arg = " <<  opt->getArgv( i ) << endl ;
	}

        /* 8. DONE */
        delete opt;

}
