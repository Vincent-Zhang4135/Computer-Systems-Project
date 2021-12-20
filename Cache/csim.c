#include "cachelab.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>


// representing a single block
// note that we do not have the byte blocks, because that does not matter
// when simulating hit or miss behavior. 
typedef struct {
    int v;
    unsigned long tag;
    int time; // keeps track of time so we know which is LRU
} block;

// struct to keep track of whether a miss, hit, and eviction has occured
// 0 will represent it has not occured, 1 will represent that it did.
typedef struct {
    int miss;
    int hit;
    int eviction;
} logistics;


// create a new_block
block newBlockLine(int v, unsigned long tag, int time) {
    block new_block;
    new_block.v = v;
    new_block.tag = tag;
    new_block.time = time;

    return new_block;
}

// create a new logistics_tracker 
logistics newTracker() {
    logistics tracker;
    tracker.miss = 0;
    tracker.hit = 0;
    tracker.eviction = 0;

    return tracker;
}


// use bit manipulation to take out the tag from the address
unsigned long extractTag(unsigned long address, int s, int b) {
    unsigned long tag = address >> (b + s);
    return tag;
}

// use bit manipulation to take out the set from the address
unsigned long extractSet(unsigned long address, int s, int b) {
    unsigned long mask = ~(~0 << s); 
    unsigned long set = (address >> b) & mask;
    return set;
}


// determines for a single instance, whether a hit, a miss, or an evict has occured.
logistics singleBlockLineSimulate(block **cache, int E, int tag, int set) {
    logistics tracker;
    tracker = newTracker();
    printf("the beginning of this block is working: ");
    int opened_i = -1;
    // loop through the E blocks in the set
    for (int i = 0; i < E; i++) {
        // check if the it is a valid block, and the tags match
        if ((!!cache[set][i].v) && (cache[set][i].tag == tag)) {
            tracker.hit = 1;
            // set it to back 0 since we just used this block
            cache[set][i].time = 0;
        } // if it is a valid bit, but just not a the matching tag, then
        else if ((!!cache[set][i].v) && (cache[set][i].tag != tag)) {
            cache[set][i].time += 1; 
        } else {
            opened_i = i;
        }
    }
    
    // now, we need to check to see if we missed, whether it is open or needs to be evicted
    if (!(tracker.hit)) {
        if (opened_i == -1) {
            tracker.eviction = 1;
            //tracker.miss = 1;

            int LRU_index = -1;
            int LRU_time = 0;

            // loop through each of the blocks to find the LRU
            for (int i = 0; i < E; i++) {
                if ((!!cache[set][i].v) && cache[set][i].time >= LRU_time) {
                    LRU_index = i;
                    LRU_time = cache[set][i].time;
                }
            }

            if (LRU_index < 0) {
                LRU_index = 0;
            }
            block new_block = newBlockLine(1, tag, 0);
            cache[set][LRU_index] = new_block;
        } else {
            tracker.miss = 1;

            if (opened_i < 0) {
                opened_i = 0;
            }
            block new_block = newBlockLine(1, tag, 0);
            cache[set][opened_i] = new_block;
        }
    }

    return tracker;
}

// function that simulates the cache 
void simulate(block **cache, int *hits, int *misses, int *evictions, FILE *trace_file, int s, int b, int E) {

    char op;
    unsigned long address;
    int size;

    while (fscanf(trace_file, " %c %lx,%d", &op, &address, &size) > 0) {
        printf("\none iteration\n");
        printf("E: %d\n", E);
        // printf(" %c %lx %d", op, address, size);
        printf("op: %c\n", op);
        if ((op == 'M') || (op == 'L') || (op == 'S')) {
            printf("s, b: %d, %d \n", s, b);
            unsigned long set = extractSet(address, s, b);
            printf("set: %lx\n", set);
            unsigned long tag = extractTag(address, s, b);
            printf("tag: %lx\n", tag);

            logistics single_logistic = singleBlockLineSimulate(cache, E, tag, set);
            if (op == 'M') {
                *hits += 1;
            }
            
            *hits += single_logistic.hit;
            *misses += single_logistic.miss;
            *evictions += single_logistic.eviction;
        }
    }

    return;
}

int main(int argc, char *argv[])
{
    
    // initializing variables
    int s, E, b;
    int hits = 0;
    int misses = 0;
    int evictions = 0;
    FILE *trace_path;

    for (int i = 0; i < argc; i++) {
        // for each of the arguments seperated by spaces, we want to check if
        // is one of -s, -E, or -b, and retrieve the number from it, for the 
        // -t one, retrieve the string, which will be the trace file we read from
        if (strcmp("-s", argv[i]) == 0) {
            s = atoi(argv[i + 1]);
        } else if (strcmp("-E", argv[i]) == 0) {
            E = atoi(argv[i + 1]);
        } else if (strcmp("-b", argv[i]) == 0) {
            b = atoi(argv[i + 1]);
        } else if (strcmp("-t", argv[i]) == 0) {
            trace_path = fopen(argv[i + 1], "r");
        }
    }
    int numSets = pow(2, s);

    // initialize cache size, which is a 2 dimensional array of blocks
    // first dimension being the number of sets, the second being the number of blocks per set (E)
    block **cache;
    cache = (block **)malloc(sizeof(block *) * numSets);
    for (int set = 0; set < numSets; set++) {
        cache[set] = (block *)malloc(sizeof(block) * E);
    }

    // set each cache line values to 0
    for (int set = 0; set < numSets; set++) {
        for (int block = 0; block < E; block++) {
            cache[set][block] = newBlockLine(0, 0, 0);
            // printf("hello: %d, %d\n", set, block);
            // printf("cache v, tag, time: %d, %d, %d\n", cache[set][block].v, cache[set][block].tag, cache[set][block].time);
        }
    }

    /*
    printf("-----------\n");
    printf("s is %d\n", s);
    printf("E is %d\n", E);*/
    

    simulate(cache, &hits, &misses, &evictions, trace_path, s, b, E);
    // the misses are not including the evictions but they should, so add
    // the two together as our actual misses:
    misses += evictions;

    //printf("hits: %d", hits);
    //printf("misses: %d", misses);
    //printf("evictions: %d", evictions);
    printSummary(hits, misses, evictions);
    fclose(trace_path);
    return 0;
}
