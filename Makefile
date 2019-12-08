all: numa numa_small_multi malloc  malloc_small_multi jemalloc jemalloc_small_multi

numa: src/numa_perf.c
	gcc -g -Werror -Wall -pedantic -O0 -o bin/numa_perf src/numa_perf.c -lnuma -lpthread

numa_small_multi: src/numa_small_multi.c
	gcc -g -Werror -Wall -pedantic -O0 -o bin/numa_small_multi src/numa_small_multi.c -lnuma -lpthread

malloc_small_multi: src/malloc_small_multi.c
	gcc -g -Werror -O0 -o bin/malloc_small_multi src/malloc_small_multi.c -lnuma -lpthread

malloc: src/malloc_perf.c
	gcc -g -Werror -Wall -pedantic -O0 -o bin/malloc_perf src/malloc_perf.c -lpthread

jemalloc: src/jemalloc_perf.c
	gcc -g -Werror -Wall -pedantic -O0 -o bin/jemalloc_perf src/jemalloc_perf.c -lnuma -lpthread -I`jemalloc-config --includedir` -L`jemalloc-config --libdir` -Wl,-rpath,`jemalloc-config --libdir` -ljemalloc `jemalloc-config --libs`

jemalloc_small_multi: src/jemalloc_small_multi.c
	gcc -g -Werror -Wall -pedantic -O0 -o bin/jemalloc_small_multi src/jemalloc_small_multi.c -lnuma -lpthread -I`jemalloc-config --includedir` -L`jemalloc-config --libdir` -Wl,-rpath,`jemalloc-config --libdir` -ljemalloc `jemalloc-config --libs`