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
  
  int event = PAPI_L1_DCM;
  retval = PAPI_query_event(event);
  handle_error(retval);
  
  int EventSet = PAPI_NULL;
  retval = PAPI_create_eventset(&EventSet);
  handle_error(retval);
  
  retval = PAPI_assign_eventset_component(EventSet, 0);
  handle_error(retval);
  retval = PAPI_num_cmp_hwctrs(0);
  printf("Number of hardware counters available: %d\n", retval); 
  retval = PAPI_add_event(EventSet, PAPI_L1_DCM);
  handle_error(retval);
  retval = PAPI_add_event(EventSet, PAPI_L1_ICM);
  handle_error(retval);
  retval = PAPI_add_event(EventSet, PAPI_L1_TCM);
  handle_error(retval);
  retval = PAPI_add_event(EventSet, PAPI_TLB_DM);
  handle_error(retval);

  PAPI_option_t opts;

  memset(&opts, 0, sizeof(opts));
  opts.granularity.def_cidx = 0;
  opts.granularity.eventset = EventSet;
  opts.granularity.granularity = PAPI_GRN_SYS;
  retval = PAPI_set_opt(PAPI_GRANUL, &opts);
  handle_error(retval);

  memset(&opts, 0, sizeof(opts));
  opts.domain.def_cidx = 0;
  opts.domain.eventset = EventSet;
  opts.domain.domain = PAPI_DOM_USER;
  retval = PAPI_set_opt(PAPI_DOMAIN, &opts);
  handle_error(retval);

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

    retval = PAPI_accum(EventSet, values);
    handle_error(retval);

    // retval = PAPI_stop(EventSet, values);
    // handle_error(retval);

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