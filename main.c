#include "papi.h"
#include "stdio.h"
#include "perror.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"

#define MAX_EVENTS 10
#define MAX_CORES 24
#define N_CORES 4

int retval;

int create_eventset(char *events[], int len) {

  /* Create EventSet with specified events */

  int EventSet = PAPI_NULL;
  retval = PAPI_create_eventset(&EventSet);
  handle_error(retval);
  
  retval = PAPI_assign_eventset_component(EventSet, 0);
  handle_error(retval);

  int i;
  for (i = 0; i < len; i++) {
    retval = PAPI_add_named_event(EventSet, events[i]);
    if (retval != PAPI_OK) {
      fprintf(stderr, "Could not add event \"%s\"\n", events[i]);
      handle_error(retval);
    }
  }

  return EventSet;
}

void attach_to_cpu_core(int EventSet, int cpu_num) {

  /* Attach to a CPU core*/

  PAPI_option_t opts;

  memset(&opts, 0, sizeof(opts));
  opts.cpu.eventset = EventSet;
  opts.cpu.cpu_num = cpu_num;
  retval = PAPI_set_opt(PAPI_CPU_ATTACH, &opts);
  handle_error(retval);
}

void start_eventsets(int EventSets[], int num) {
  int i;
  for (i = 0; i < num; i++) {
    retval = PAPI_start(EventSets[i]);
    if (retval != PAPI_OK) {
      fprintf(stderr, "Could not start EventSet[%d]\n", i);
      handle_error(retval);
    }
  }
}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "%s\n", "Usage: ./main [EVENTS]");
    exit(1);
  }
  char **events = argv+1;
  int n_events = argc-1;

  retval = PAPI_library_init(PAPI_VER_CURRENT);
  if (retval != PAPI_VER_CURRENT) {
    fprintf(stderr, "%s\n", "Library initialization failed");
    exit(1);
  }
  retval = PAPI_num_cmp_hwctrs(0);
  printf("Number of hardware counters available: %d\n", retval);
  if (retval < 3) {
    fprintf(stderr, "%s\n", "Not enough counters to count events");
    exit(1);
  }

  int EventSets[N_CORES];
  int i;
  for (i = 0; i < N_CORES; i++) {
    EventSets[i] = create_eventset(events, n_events);
    attach_to_cpu_core(EventSets[i], i);
  }
  start_eventsets(EventSets, N_CORES);
  
  long_long values[MAX_EVENTS];

  while (1) {
    sleep(1);

    for (i = 0; i < N_CORES; i++) {
      retval = PAPI_read(EventSets[i], values);
      if (retval != PAPI_OK) {
        fprintf(stderr, "Could not read from core %d\n", i);
        handle_error(retval);
      }

      int j;
      printf("[ Core #%d ] ", i);
      for (j = 0; j < n_events; j++) {
        printf("%s: %lld, ", events[j], values[j]);
      }
      printf("\n");
    }
    printf("\n");
    fflush(stdout);
  }
  return 0;
}
