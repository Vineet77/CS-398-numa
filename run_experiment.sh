#!/bin/bash
# A sample Bash script, by Ryan

STATS=("cpu-cycles"
    "node-loads"
    "node-load-misses"
    "node-stores"
    "node-store-misses"
    "node-prefetches"
    "node-prefetch-misses"
    "offcore_response.all_demand_mlc_pref_reads.llc_miss.local_dram"
    "offcore_response.demand_code_rd.llc_miss.local_dram"
    "offcore_response.demand_data_rd.llc_miss.local_dram"
    "offcore_response.pf_l2_data_rd.llc_miss.local_dram"
    "offcore_response.demand_code_rd.llc_miss.remote_dram"
    "offcore_response.demand_data_rd.llc_miss.remote_dram"
    "offcore_response.pf_l2_data_rd.llc_miss.remote_dram"
    "offcore_response.demand_data_rd.llc_miss.remote_hit_forward"
    "offcore_response.demand_data_rd.llc_miss.remote_hitm"
    "mem_load_uops_llc_miss_retired.local_dram"
    "mem_load_uops_llc_miss_retired.remote_dram"
    )

SIZES=$(for x in {0..17}; do echo $((4096 * (2 ** $x)));  done)
NUM_THREADS=$(for x in {0..16}; do echo $(($x * 4));  done)


#SIZES=(8000000000)
echo "${SIZES[@]}"

PROGRAMS=("$1")tmux
FNAME="output/$2"
echo "$1"

TEMP_FILE="output.txt"

function join_by { local IFS="$1"; shift; echo "$*"; }

function get_output(){
    vals=($2)
    vals+=($3)
    while IFS= read -r line; do
        IFS=',' read -r -a array <<< "$line"
        val=${array[0]}
        #check if a non-number was given
        re='^[0-9]+$'
        if ! [[ $val =~ $re ]] ; then
            val="0"
        fi
        vals+=($val)
        echo "$val,"
    done < <(tail -n ${#STATS[@]} $TEMP_FILE)  #skip the first two lines
    echo "${vals[@]}" >> $1
}

function run() {
    echo "running $1"
    
    #fname="output/$1.txt"
    echo "output to $FNAME"

    echo "# size thread(s) ${STATS[@]}" > $FNAME

    for i in ${SIZES}; do
        for j in ${NUM_THREADS}; do 
            if [ "$j" -eq "0" ]; then
                j=1
            fi
            echo "(perf stat -x , -r 10 -o $TEMP_FILE -e $(join_by , ${STATS[@]}) ./bin/$1 -s $i -t $j)"
            result=$(perf stat -x , -r 10 -o $TEMP_FILE -e $(join_by , ${STATS[@]}) ./bin/$1 -s $i -t $j)
            get_output $FNAME $i $j
            echo $result
        done

    done
}
 

run "$1"


