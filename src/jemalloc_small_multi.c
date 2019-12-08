#define _GNU_SOURCE
#include <numa.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <getopt.h>
#include <sched.h>

#define ONE_NODE 0
#define MULTI_NODE 1
#define OTHER_NODE 2
#define ANY_NODE 3

#define NUM_NODES 4

long long size = 4096;
int memset_num = 10;
int node_mode = ANY_NODE;

const struct option long_options[] = {{"size", required_argument, 0, 's'},        {"memset", required_argument, 0, 'm'},
                                      {"multithread", required_argument, 0, 't'}, {"node", required_argument, 0, 'n'},
                                      {"use_cache", no_argument, 0, 'c'},         {0, 0, 0, 0}};
static cpu_set_t cpusets[4];

static int num_threads = 1;
static int cache = 0;

const int others[4] = {2, 3, 0, 1};

void read_write(char* ptr, long long size) {
    for (int i = 0; i < memset_num; i++) {
        for (long long j = 0; j < size; j++) {
            ptr[j] += 1;
        }
    }
}

void read_write_non_cache(char* ptr, long long size) {
    for (int i = 0; i < memset_num; i++) {
        for (long long j = 0; j < size; j++) {
            *(ptr + ((j * 1009) % size)) += 1;
        }
    }
}

void get_node_info() {
    for (int i = 0; i < NUM_NODES; i++) {
        CPU_ZERO(&cpusets[i]);
    }

    int num_cpus = numa_num_possible_cpus();
    for (int i = 0; i < num_cpus; i++) {
        int node = numa_node_of_cpu(i);
        CPU_SET(i, &cpusets[node]);
        printf("cpu %d belongs to %d\n", i, node);
    }
}

void* allocate(void* arg) {
    int node = *(int*)arg;
    if (node_mode == ONE_NODE)
        node = 0;
    else if (node_mode == OTHER_NODE)
        node = others[node];

    if (node_mode != ANY_NODE) numa_run_on_node(node);
    // numa_set_strict(1);

    char* allocated = (char*)malloc(size);

    if (cache)
        read_write(allocated, size);
    else
        read_write_non_cache(allocated, size);

    return 0;
}

int main(int argc, char* argv[]) {
    int val;
    int option_index = 0;

    while ((val = getopt_long(argc, argv, "s:m:t:n:c", long_options, &option_index)) != -1) {
        switch (val) {
            case 's':
                size = atoll(optarg);
                break;
            case 'm':
                memset_num = atoi(optarg);
                break;
            case 't':
                num_threads = atoi(optarg);
                break;
            case 'c':
                cache = 1;
                break;
            case 'n':
                switch (atoi(optarg)) {
                    case MULTI_NODE:
                        node_mode = MULTI_NODE;
                        break;
                    case OTHER_NODE:
                        node_mode = OTHER_NODE;
                        break;
                    case ONE_NODE:
                        node_mode = ONE_NODE;
                        break;
                }
                break;
        }
    }
    printf("Using allocation size of %lld, memset num of %d\n", size, memset_num);
    if (num_threads) printf("Using %d threads\n", num_threads);

    pthread_t threads[num_threads];
    int args[num_threads];

    for (int i = 0; i < num_threads; i++) {
        args[i] = i;
        int val = pthread_create(&threads[i], NULL, allocate, &args[i]);
        if (val != 0) {
            printf("Failed to created thread %d\n", i);
        }
    }

    if (!num_threads) exit(0);
    // wait for all threads to complete
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
}