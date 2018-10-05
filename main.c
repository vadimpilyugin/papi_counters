#include "papi.h"
#include "stdio.h"
#include "perror.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"

int retval;
struct loop_params {
  int n_cores;
  long long *values;
  int *EventSets;
  char **events;
  int n_events;
};

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

int how_many_cores() {
  const PAPI_hw_info_t *info = PAPI_get_hardware_info();
  if (info != NULL) {
    return info -> totalcpus;
  } else {
    fprintf(stderr, "Hardware info is unavailable\n");
    exit(1);
  }
}

void print_usage() {
  fprintf(stderr, "%s\n", "Usage: ./main [EVENTS]");
  exit(1);
}

struct loop_params initialize(int argc, char **argv) {
  if (argc < 2) {
    print_usage();
  }
  char **events = argv + 1;
  int n_events = argc - 1;

  retval = PAPI_library_init(PAPI_VER_CURRENT);
  if (retval != PAPI_VER_CURRENT) {
    fprintf(stderr, "%s\n", "Library initialization failed");
    exit(1);
  }

  int n_cores = how_many_cores();
  retval = PAPI_num_cmp_hwctrs(0);
  printf("Number of hardware counters available: %d\n", retval);
  if (retval < n_cores * n_events) {
    fprintf(stderr, "Not enough counters: need %d\n", n_cores * n_events);
    exit(1);
  }

  int *EventSets = (int *) malloc(n_cores * sizeof(int));
  int i;
  for (i = 0; i < n_cores; i++) {
    EventSets[i] = create_eventset(events, n_events);
    attach_to_cpu_core(EventSets[i], i);
  }
  start_eventsets(EventSets, n_cores);

  struct loop_params lp;
  lp.n_cores = n_cores;
  lp.values = (long long *) malloc (n_events * sizeof(long long));
  lp.EventSets = EventSets;
  lp.events = events;
  lp.n_events = n_events;
  return lp;
}

void loop(struct loop_params lp) {
  sleep(1);

  int i;
  for (i = 0; i < lp.n_cores; i++) {

    retval = PAPI_read(lp.EventSets[i], lp.values);
    if (retval != PAPI_OK) {
      fprintf(stderr, "Could not read from core %d\n", i);
      handle_error(retval);
    }

    printf("[ Core #%d ] ", i);

    int j;
    for (j = 0; j < lp.n_events; j++) {
      printf("%s: %lld, ", lp.events[j], lp.values[j]);
    }

    printf("\n");
  }
  printf("\n");
  fflush(stdout);
}

int main(int argc, char **argv) {
  struct loop_params lp = initialize(argc, argv);
  while (1) {
    loop(lp);
  }
  return 0;
}
