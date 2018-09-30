#include "papi.h"
#include "stdio.h"
#include "perror.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "%s\n", "Usage: ./main [CPU TO TRACE]");
    exit(1);
  }

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
  // PAPI_L1_TCM  0x80000006  Yes  Level 1 cache misses
  // PAPI_TOT_INS 0x80000032  No   Instructions completed
  // PAPI_TLB_DM  0x80000014  Yes   Yes  Data translation lookaside buffer misses

  retval = PAPI_add_event(EventSet, PAPI_L1_TCM);
  handle_error(retval);
  retval = PAPI_add_event(EventSet, PAPI_TOT_INS);
  handle_error(retval);
  retval = PAPI_add_event(EventSet, PAPI_TLB_DM);
  handle_error(retval);

  /* Attach to CPU core 0 */

  PAPI_option_t opts;

  memset(&opts, 0, sizeof(opts));
  opts.cpu.eventset = EventSet;
  opts.cpu.cpu_num = atoi(argv[1]);
  retval = PAPI_set_opt(PAPI_CPU_ATTACH, &opts);
  handle_error(retval);

  // pid_t pid = atoi(argv[1]);
  // retval = PAPI_attach(EventSet, (unsigned long) pid);
  // handle_error(retval);
  // printf("Attached to process: %d\n", pid);

  long_long values[3];
  retval = PAPI_start(EventSet);
  handle_error(retval);

  while (1) {
    sleep(1);

    retval = PAPI_read(EventSet, values);
    handle_error(retval);

    printf("L1_TCM: %lld, ",values[0]);
    printf("TOT_INS: %lld, ",values[1]);
    printf("TLB_DM: %lld\n",values[2]);
    fflush(stdout);
  }

  printf("Hello, world!\n");
  return 0;
}
