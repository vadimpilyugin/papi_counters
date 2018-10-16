#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
// #include <stdbool.h>
// #include <stdio.h>

#include "dmm_base.h"
#include "dmm_log.h"
#include "dmm_message.h"
// #include "sensors.h"
#include "papi.h"

#define CPU_COMPONENT 0
// #define DATAFILE "/proc/meminfo"

// struct pvt_data {
//     FILE *f;
//     dmm_hook_p hook;
// };

int retval;
struct loop_params {
  int n_cores;
  long long *values;
  int *EventSets;
  char **events;
  int n_events;
};

struct pvt_data {
    struct loop_params lp;
    dmm_hook_p hook;
};

void handle_error(int retval)
{
  if (retval != PAPI_OK) {
    printf("PAPI error %d: %s\n", retval, PAPI_strerror(retval));
    exit(1);
  }
}

void perror(const char *str) {
  fprintf(stderr, "%s\n", str);
  exit(1);
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
  }
  perror("Hardware info is unavailable");
  return -1;
}

void initialize(int n_events, char *events[], struct loop_params *lp) {

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

  lp -> n_cores = n_cores;
  lp -> values = (long long *) DMM_MALLOC (n_events * sizeof(long long));
  lp -> EventSets = EventSets;
  lp -> events = events;
  lp -> n_events = n_events;
}

// struct search_item_t {
//     const char *header;
//     dmm_id_t    sensor_id;
//     bool        convert_from_k;
// } search_list[] = {
//     { "MemTotal",           MEMORY_MEMTOTAL,            true },
//     { "MemFree",            MEMORY_MEMFREE,             true },
//     { "MemAvailable",       MEMORY_MEMAVAILABLE,        true },
//     { "Buffers",            MEMORY_BUFFERS,             true },
//     { "Cached",             MEMORY_CACHED,              true },
//     { "Active",             MEMORY_ACTIVE,              true },
//     { "Inactive",           MEMORY_INACTIVE,            true },
//     { "Mlocked",            MEMORY_MLOCKED,             true },
//     { "AnonPages",          MEMORY_ANONPAGES,           true },
//     { "Mapped",             MEMORY_MAPPED,              true },
//     { "Shmem",              MEMORY_SHMEM,               true },
//     { NULL,                 0,                          false }
// };

// const int num_sensors = sizeof(search_list) / sizeof(*search_list) - 1;

// XXX- dirty hack to make memory sensors work with avgcollector
//typedef unsigned long int sensor_type;
// typedef float sensor_type;

typedef long long[2] sensor_type;

void loop(struct loop_params lp) {
  int i;
  for (i = 0; i < lp.n_cores; i++) {

    retval = PAPI_read(lp.EventSets[i], lp.values); // writing counters to lp.values
    if (retval != PAPI_OK) {
      fprintf(stderr, "Could not read from core %d\n", i);
      handle_error(retval);
    }

    // printf("[ Core #%d ] ", i);

    // int j;
    // for (j = 0; j < lp.n_events; j++) {
    //   printf("%s: %lld, ", lp.events[j], lp.values[j]);
    // }

    // printf("\n");
  }
  // printf("\n");
  // fflush(stdout);
}

static int process_timer_msg(dmm_node_p node)
{
    // reading values

    // struct pvt_data *pvt;
    // FILE *f;
    // char buf[64];
    dmm_datanode_p dn;
    dmm_data_p data;
    // int sensors_found;
    struct loop_params lp;

    pvt = (struct pvt_data *)DMM_NODE_PRIVATE(node);
    // data = NULL;
    // f = pvt->f;
    // fflush(f);
    // rewind(f);
    lp = pvt -> lp;

    // data = DMM_DATA_CREATE(num_sensors, sizeof(sensor_type));
    data = DMM_DATA_CREATE(1, lp.n_events * sizeof(long long));

    if (data == NULL) {
        return ENOMEM;
    }
    dn = DMM_DATA_NODES(data);

    // sensors_found = 0;

    // while ((sensors_found < num_sensors) && !feof(f)) {
//         int retval;
        sensor_type value;
//         struct search_item_t *s;

//         // XXX- dirty hack to make memory sensors work with avgcollector
// //        retval = fscanf(f, "%60[^:]: %lu%*[^\n]%*c", buf, &value);
//         retval = fscanf(f, "%60[^:]: %f%*[^\n]%*c", buf, &value);
//         if (retval < 2 || retval == EOF) {
//             assert(fscanf(f, "%*[^\n]%*c") == 0);
//             break;
//         }
//         for (s = search_list; s->header != NULL; ++s) {
//             if (strncmp(buf, s->header, sizeof(buf)) == 0)
//                 break;
//         }
//         if (s->header == NULL)
//             continue;


        // DMM_DN_CREATE(dn, PAPI_COUNTERS, sizeof(sensor_type));
        DMM_DN_CREATE(dn, PAPI_COUNTERS, sizeof(sensor_type));
        // *DMM_DN_DATA(dn, sensor_type) = value * (s->convert_from_k ? 1024 : 1);
        *DMM_DN_DATA(dn, sensor_type) = value * (s->convert_from_k ? 1024 : 1);
        // DMM_DN_ADVANCE(dn);


        // ++sensors_found;
    // }
    DMM_DN_MKEND(dn);

    if (sensors_found > 0)
        DMM_DATA_SEND(data, pvt->hook);

    DMM_DATA_UNREF(data);
    return 0;
}

static int ctor(dmm_node_p node)
{
    // constructor

    struct pvt_data *pvt;
    int err;

    pvt = (struct pvt_data *)DMM_MALLOC(sizeof(*pvt));
    if (pvt == NULL)
        return ENOMEM;
    DMM_NODE_SETPRIVATE(node, pvt);

    int n_events = 2;
    char *events[] = {"PAPI_L1_TCM", "PAPI_TOT_CYC"};
    initialize(n_events, events, &(pvt -> lp));
    

    // pvt->f = fopen(DATAFILE, "re");
    // if (pvt->f == NULL) {
    //     char errbuf[128], *errmsg;
    //     err = errno;
    //     errmsg = strerror_r(err, errbuf, sizeof(errbuf));
    //     dmm_log(DMM_LOG_ERR, "Cannot open %s for reading: %s", DATAFILE, errmsg);
    //     DMM_FREE(pvt);
    //     return err;
    // }
    // // Disable buffering to have fresh data on successive reads
    // setlinebuf(pvt->f);
    pvt->hook = NULL;
    return 0;
}

static void dtor(dmm_node_p node)
{
    // destructor

    struct pvt_data *pvt;
    pvt = (struct pvt_data *)DMM_NODE_PRIVATE(node);

    DMM_FREE(pvt -> lp.values);
    DMM_FREE(pvt -> lp.EventSets);

    DMM_FREE(pvt);
}

static int newhook(dmm_hook_p hook)
{
    struct pvt_data *pvt;

    if (DMM_HOOK_ISIN(hook))
        return EINVAL;
    // if (strcmp("out", DMM_HOOK_NAME(hook)) != 0)
    //     return EINVAL;
    pvt = (struct pvt_data *)DMM_NODE_PRIVATE(DMM_HOOK_NODE(hook));
    pvt->hook = hook;

    return 0;
}

static void rmhook(dmm_hook_p hook)
{
    struct pvt_data *pvt;
    pvt = (struct pvt_data *)DMM_NODE_PRIVATE(DMM_HOOK_NODE(hook));
    pvt->hook = NULL;
}

static int rcvmsg(dmm_node_p node, dmm_msg_p msg)
{
    // do not change

    int err = 0;

    /* Accept only generic messages */
    if (msg->cm_type != DMM_MSGTYPE_GENERIC) {
        err = ENOTSUP;
        goto err;
    }
    /* Accept only TIMERTRIGGER messages */
    if (msg->cm_cmd != DMM_MSG_TIMERTRIGGER) {
        err = ENOTSUP;
        goto err;
    }
    err = process_timer_msg(node);

err:
    DMM_MSG_FREE(msg);
    return err;

}
static struct dmm_type type = {
    "memory",
    ctor,
    dtor,
    NULL,
    rcvmsg,
    newhook,
    rmhook,
    {},
};

DMM_MODULE_DECLARE(&type);
