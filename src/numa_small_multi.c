#include <numa.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>

#define ONE_NODE 0
#define MULTI_NODE 1
#define OTHER_NODE 2

long long size = 4096;
int memset_num = 10;
int t = 33;
int alloc_num;
int node_mode = MULTI_NODE;

const struct option long_options[] = {{"size", required_argument, 0, 's'},
                                      {"memset", required_argument, 0, 'm'},
                                      {"multithread", no_argument, 0, 't'},
                                      {"node", required_argument, 0, 'n'},
                                      {0, 0, 0, 0}};
const int others[4] = {2, 3, 0, 1};

int pow2(long long val) {
    int p = 0;
    while (val >>= 1) ++p;
    return p;
}

void* allocate(void* arg) {
    int node = *(int*)arg;
    int allocation_ndoe = *(int*)arg;
    if (node_mode == ONE_NODE)
        node = 1;
    else if (node_mode == OTHER_NODE)
        node = others[node];

    numa_run_on_node(node);
    numa_set_strict(1);

    printf("Running on node %d, allocating on node %d\n", node, allocation_ndoe);

    int i = 0, j = 0;
    for (i = 0; i < alloc_num; i++) {
        char* ptr = (char*)numa_alloc_onnode(size, node);
        if (ptr == NULL) {
            printf("Numa allocate on node %d failed on %d allocation. Error %d: %s\n", node, i, errno, strerror(errno));
            long long free = 0;
            numa_node_size64(node, &free);
            printf("%lld bytes free on node %d\n", free, node);
            break;
        }
        if (errno == ENOMEM) {
            printf("numa allocate caused a ENOMEN on node %d, allocation #%d. Error: %s\n", node, i, strerror(errno));
            long long free = 0;
            numa_node_size64(node, &free);
            printf("%lld bytes free on node %d\n", free, node);
            break;
        }
        if (errno != 0) {
            printf("Numa allocate lead to errno %d: %s. Node: %d, allocation #%d\n", errno, strerror(errno), node, i);
            break;
        }

        for (j = 0; j < memset_num; j++) {
            memset(ptr, j, size);
        }
    }

    return 0;
}

int main(int argc, char* argv[]) {
    // parse command line arugments
    int val;
    int option_index = 0;
    int mutltread = 0;

    while ((val = getopt_long(argc, argv, "s:m:tn:", long_options, &option_index)) != -1) {
        switch (val) {
            case 's':
                size = atoll(optarg);
                break;
            case 'm':
                memset_num = atoi(optarg);
                break;
            case 't':
                mutltread = 1;
                break;
            case 'n':
                switch (atoi(optarg)) {
                    case MULTI_NODE:
                        node_mode = MULTI_NODE;
                        break;
                    case OTHER_NODE:
                        node_mode = OTHER_NODE;
                        break;
                    default:
                        node_mode = ONE_NODE;
                        break;
                }
                break;
        }
    }
    alloc_num = 1 << (t - pow2(size));
    printf("Using allocation size of %lld and %d number of allocations\n", size, alloc_num);

    int num = numa_num_configured_nodes();
    pthread_t threads[num];
    int args[num];

    for (int i = 0; i < num; i++) {
        args[i] = i;
        if (mutltread) {
            int val = pthread_create(&threads[i], NULL, allocate, &args[i]);
            if (val != 0) {
                printf("Failed to created thread %d\n", i);
            }
        } else {
            allocate(&args[i]);
        }
    }
    if (!mutltread) exit(0);

    // wait for all threads to complete
    for (int i = 0; i < num; i++) {
        pthread_join(threads[i], NULL);
    }
}