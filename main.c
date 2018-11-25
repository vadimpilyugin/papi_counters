#include "papi.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"

#define CPU_COMPONENT 0
#define DMM_MALLOC(size) malloc( (size) )
#define DMM_FREE(ptr) free(ptr)

int retval;
struct loop_params {
  int n_events;
  int n_cores;
  char **events; // 0..n_events-1
  long long **values; // 0..n_cores-1
  int *EventSets; // 0..n_cores-1
};

void handle_error (int retval)
{
  if (retval != PAPI_OK) {
    printf("PAPI error %d: %s\n", retval, PAPI_strerror(retval));
    exit(1);
  }
}

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

struct loop_params * initialize(int n_events, char *events[]) {

  retval = PAPI_library_init(PAPI_VER_CURRENT);
  if (retval != PAPI_VER_CURRENT) {
    fprintf(stderr, "%s\n", "Library initialization failed");
    handle_error(retval);
  }

  int n_cores = how_many_cores();
  retval = PAPI_num_cmp_hwctrs(CPU_COMPONENT);
  printf("Number of hardware counters available: %d\n", retval);
  if (retval < n_cores * n_events) {
    fprintf(stderr, "Not enough counters: need %d\n", n_cores * n_events);
    exit(1);
  }

  int *EventSets = (int *) DMM_MALLOC(n_cores * sizeof(int));
  int i;
  for (i = 0; i < n_cores; i++) {
    EventSets[i] = create_eventset(events, n_events);
    attach_to_cpu_core(EventSets[i], i);
  }
  start_eventsets(EventSets, n_cores);

  struct loop_params * lp = (struct loop_params *) DMM_MALLOC(sizeof(struct loop_params));
  lp -> n_cores = n_cores;
  lp -> values = (long long **) DMM_MALLOC(n_cores * sizeof(long long *));
  for (i = 0; i < n_cores; i++)
    lp -> values[i] = (long long *) DMM_MALLOC(n_events * sizeof(long long));
  lp -> EventSets = EventSets;
  lp -> events = events;
  lp -> n_events = n_events;
  return lp;
}

void free_lp(struct loop_params *lp) {
  DMM_FREE(lp -> EventSets);
  int i = 0;
  for (i = 0; i < lp -> n_cores; i++)
    DMM_FREE(lp -> values[i]);
  DMM_FREE(lp -> values);
  DMM_FREE(lp);
}

void print_values(struct loop_params *lp) {
  int i,j;
  printf("\n");
  for (i = 0; i < lp -> n_cores; i++) {
    printf("[ Core #%d ] ", i);
    for (j = 0; j < lp -> n_events; j++) {
      if (j > 0)
        printf(", ");
      printf("%s: %lld", lp -> events[j], lp -> values[i][j]);      
    }
    printf("\n");
  }
  fflush(stdout);
}

void read_values(struct loop_params *lp) {
  int i;
  for (i = 0; i < lp -> n_cores; i++) {
    retval = PAPI_read(lp -> EventSets[i], lp -> values[i]);
    if (retval != PAPI_OK) {
      fprintf(stderr, "Could not read from core %d\n", i);
      handle_error(retval);
    }
  }
}

void loop(struct loop_params *lp) {
  sleep(1);
  read_values(lp);
  print_values(lp);
}

int main(int argc, char **argv) {
  if (argc < 2) {
    print_usage();
  }
  char **events = argv + 1;
  int n_events = argc - 1;
  struct loop_params * lp = initialize(n_events, events);
  while (1) {
    loop(lp);
  }
  free_lp(lp);
  return 0;
}
