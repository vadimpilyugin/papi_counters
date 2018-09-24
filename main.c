#include "papi.h"
#include "stdio.h"
#include "perror.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"

int main() {
  int retval = PAPI_library_init(PAPI_VER_CURRENT);
  if (retval != PAPI_VER_CURRENT) {
    fprintf(stderr, "%s\n", "Library initialization failed");
    exit(1);
  }
  
  int EventSet = PAPI_NULL;
  retval = PAPI_create_eventset(&EventSet);
  handle_error(retval);
  
  retval = PAPI_assign_eventset_component(EventSet, 0);
  handle_error(retval);

  retval = PAPI_num_cmp_hwctrs(0);
  printf("Number of hardware counters available: %d\n", retval);
  if (retval < 3) {
    fprintf(stderr, "%s\n", "Not enough counters to count events");
    exit(1);
  }

  // Добавляем события, которые хотим считать:
  // PAPI_L1_DCM  0x80000000  Yes   No   Level 1 data cache misses
  // PAPI_L1_ICM  0x80000001  Yes   No   Level 1 instruction cache misses
  // PAPI_TLB_DM  0x80000014  Yes   Yes  Data translation lookaside buffer misses

  retval = PAPI_add_event(EventSet, PAPI_L1_DCM);
  handle_error(retval);
  retval = PAPI_add_event(EventSet, PAPI_L1_ICM);
  handle_error(retval);
  retval = PAPI_add_event(EventSet, PAPI_TLB_DM);
  handle_error(retval);

  PAPI_option_t opts;

  memset(&opts, 0, sizeof(opts));
  opts.cpu.eventset = EventSet;
  opts.cpu.cpu_num = 0;
  retval = PAPI_set_opt(PAPI_CPU_ATTACH, &opts);
  handle_error(retval);


  long_long values[4];
  while (1) {
    retval = PAPI_start(EventSet);
    handle_error(retval);

    sleep(1);

    retval = PAPI_stop(EventSet, values);
    handle_error(retval);

    // PAPI_reset() zeroes the values of the counters contained in EventSet
    retval = PAPI_reset(EventSet);
    handle_error(retval);

    printf("L1_DCM: %lld, ",values[0]);
    printf("L1_ICM: %lld, ",values[1]);
    printf("L1_TCM: %lld, ",values[2]);
    printf("TLB_DM: %lld\n",values[3]);
    memset(values, 0, sizeof(values));
  }

  printf("Hello, world!\n");
  return 0;
}