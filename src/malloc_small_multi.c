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
#include <numa.h>
#include <getopt.h>

#define ONE_NODE 0
#define MULTI_NODE 1
#define OTHER_NODE 2
#define ANY_NODE 3

#define NUM_NODES 4
#define MAX_ALLOCATIONS 262144

pthread_mutex_t lock;
static __thread char* pointers[131072];

long long size = 4096;
int memset_num = 10;
int t = 29;
int alloc_num;
int node_mode = ANY_NODE;

const struct option long_options[] = {{"size", required_argument, 0, 's'},        {"memset", required_argument, 0, 'm'},
                                      {"multithread", required_argument, 0, 't'}, {"node", required_argument, 0, 'n'},
                                      {"use_cache", no_argument, 0, 'c'},         {0, 0, 0, 0}};

static int num_threads = 1;
static int cache = 0;

const int others[4] = {2, 3, 0, 1};

char* ptrs[MAX_ALLOCATIONS];

int pow2(long long val) {
    int p = 0;
    while (val >>= 1) ++p;
    return p;
}

void read_write(char* ptr, long long size) {
    for (long long j = 0; j < size; j++) {
        ptr[j] += 1;
    }
}

void read_write_non_cache(char* ptr, long long size) {
    for (long long j = 0; j < size; j++) {
        *(ptr + ((j * 1009) % size)) += 1;
    }
}

void* allocate(void* arg) {
    int node = *(int*)arg;
    numa_run_on_node(node);
    char* allocated = (char*)malloc(size);

    for (int i = 0; i < alloc_num; i++) {
        char* ptr = (char*)malloc(size);
        // printf("pointers[%d] = %p\n", i, (void*)ptr);
        pointers[i] = ptr;
    }

    for (int i = 0; i < memset_num; i++) {
        // printf("memset #%d\n", i);
        for (int j = 0; j < alloc_num; j++) {
            if (cache)
                read_write(pointers[j], size);
            else
                read_write_non_cache(pointers[j], size);
        }
        if (i == 0)  // switch nodes
            numa_run_on_node(others[node]);
    }
    return 0;
}

int main(int argc, char* argv[]) {
    // parse command line arugments
    int val;
    int option_index = 0;
    int mutltread = 0;

    if (pthread_mutex_init(&lock, NULL) != 0) {
        printf("\n mutex init has failed\n");
        return 1;
    }

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
    alloc_num = 1 << (t - pow2(size));
    printf("%ld\n", (long)getpid());
    printf("Using allocation size of %lld and %d number of allocations\n", size, alloc_num);

    printf("Using allocation size of %lld, memset num of %d\n", size, memset_num);
    if (num_threads) printf("Using %d threads\n", num_threads);

    pthread_t threads[num_threads];
    int args[num_threads];
    int num = numa_num_configured_nodes();

    for (int i = 0; i < num_threads; i++) {
        args[i] = i % num;
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

    pthread_mutex_destroy(&lock);
}