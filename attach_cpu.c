/*
 * This test case creates an event set and attaches it to a cpu.  This causes only activity
 * on that cpu to get counted.	The test case then starts the event set does a little work and
 * then stops the event set.  It then prints out the event, count and cpu number which was used
 * during the test case.
 *
 * Since this test case does not try to force its own execution to the cpu which it is using to
 * count events, it is fairly normal to get zero counts printed at the end of the test.	 But every
 * now and then it will count the cpu where the test case is running and then the counts will be non-zero.
 *
 * The test case allows the user to specify which cpu should be counted by providing an argument to the
 * test case (ie: ./attach_cpu 3).  Sometimes by trying different cpu numbers with the test case, you
 * can find the cpu used to run the test (because counts will look like cycle counts).
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "papi.h"
// #include "papi_test.h"

/* Use a positive value of retval to simply print an error message */
void
test_fail( const char *file, int line, const char *call, int retval )
{
//	int line_pad;
	char buf[128];

	(void)file;

//	line_pad=(60-strlen(file));
//	if (line_pad<0) line_pad=0;

//	fprintf(stdout,"%s",file);
//	print_spaces(line_pad);

	memset( buf, '\0', sizeof ( buf ) );

	if ( retval == PAPI_ESYS ) {
		sprintf( buf, "System error in %s", call );
		perror( buf );
	} else if ( retval > 0 ) {
		fprintf( stdout, "Error: %s\n", call );
	} else if ( retval == 0 ) {
#if defined(sgi)
		fprintf( stdout, "SGI requires root permissions for this test\n" );
#else
		fprintf( stdout, "Error: %s\n", call );
#endif
	} else {
		fprintf( stdout, "Error in %s: %s\n", call, PAPI_strerror( retval ) );
	}

//	fprintf( stdout, "\n" );

	/* NOTE: Because test_fail is called from thread functions,
	   calling PAPI_shutdown here could prevent some threads
	   from being able to free memory they have allocated.
	 */
	if ( PAPI_is_initialized(  ) ) {
		PAPI_shutdown(  );
	}

	/* This is stupid.  Threads are the rare case */
	/* and in any case an exit() should clear everything out */
	/* adding back the exit() call */

	exit(1);
}

int
main( int argc, char **argv )
{
	int retval;
	int cpu_num = 1;
	int EventSet1 = PAPI_NULL;
	long long values[1];
	char event_name[PAPI_MAX_STR_LEN] = "PAPI_TOT_CYC";
	PAPI_option_t opts;
	int quiet = 0;


	// user can provide cpu number on which to count events as arg 1
	if (argc > 1) {
		retval = atoi(argv[1]);
		if (retval >= 0) {
			cpu_num = retval;
		}
	}

	retval = PAPI_library_init( PAPI_VER_CURRENT );
	if ( retval != PAPI_VER_CURRENT )
		test_fail( __FILE__, __LINE__, "PAPI_library_init", retval );

	retval = PAPI_create_eventset(&EventSet1);
	if ( retval != PAPI_OK )
		test_fail( __FILE__, __LINE__, "PAPI_attach", retval );

	// Force event set to be associated with component 0 (perf_events component provides all core events)
	retval = PAPI_assign_eventset_component( EventSet1, 0 );
	if ( retval != PAPI_OK )
		test_fail( __FILE__, __LINE__, "PAPI_assign_eventset_component", retval );

	// Attach this event set to cpu 1
	opts.cpu.eventset = EventSet1;
	opts.cpu.cpu_num = cpu_num;

	retval = PAPI_set_opt( PAPI_CPU_ATTACH, &opts );
	if ( retval != PAPI_OK )
		test_fail( __FILE__, __LINE__, "PAPI_set_opt", retval );

	retval = PAPI_add_named_event(EventSet1, event_name);
	if ( retval != PAPI_OK ) {
		if (!quiet) printf("Trouble adding event %s\n",event_name);
		test_fail( __FILE__, __LINE__, "PAPI_add_named_event", retval );
	}

	while(1) {
		retval = PAPI_start( EventSet1 );
		if ( retval != PAPI_OK ) {
			test_fail( __FILE__, __LINE__, "PAPI_start", retval );
		}

		// sleep for a while
	    sleep(1);

		retval = PAPI_stop( EventSet1, values );
		if ( retval != PAPI_OK )
			test_fail( __FILE__, __LINE__, "PAPI_stop", retval );

		if (!quiet) printf ("Event: %s: %8lld on Cpu: %d\n", event_name, values[0], cpu_num);

		retval = PAPI_reset( EventSet1 );
		if ( retval != PAPI_OK )
			test_fail( __FILE__, __LINE__, "PAPI_reset", retval );
	}

	PAPI_shutdown( );

	return 0;

}
