#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>

#if 0 // SANITY
#define PRINT_ITERATION_INTERVAL     1
#define ITER_STOP(iter)              (iter < 5)
#else
#define PRINT_ITERATION_INTERVAL     (1<<22)
#define ITER_STOP(iter)
#endif

#define COUNT_LEAST_USED_BUCKET_INDEX(iter)      ((iter) > 8*PRINT_ITERATION_INTERVAL)

#define DIRECT_CAPACITY_BITSHIFT     38
#define DIRECT_BUCKET_SIZE_BITSHIFT  13

#define DIRECT_BUCKET_COUNT_BITSHIFT (DIRECT_CAPACITY_BITSHIFT - DIRECT_BUCKET_SIZE_BITSHIFT)

#define AVG_SPARE_IN_BLOCK       0.75
#define UNITS_IN_BLOCK           8lu

#define PHYSICAL_BLOCKS          (1lu << DIRECT_BUCKET_COUNT_BITSHIFT) /* total */
#define SPARE_PHYSICAL_IN_BLOCKS ((unsigned long)(AVG_SPARE_IN_BLOCK * PHYSICAL_BLOCKS / UNITS_IN_BLOCK))
#define USED_PHYSICAL_IN_BLOCKS  (PHYSICAL_BLOCKS - SPARE_PHYSICAL_IN_BLOCKS)
#define USED_UNITS               (UNITS_IN_BLOCK * USED_PHYSICAL_IN_BLOCKS)

#define INITIAL_BLOCKS_BY_USED                                          \
    {                                                                   \
        [UNITS_IN_BLOCK] = PHYSICAL_BLOCKS*(1-AVG_SPARE_IN_BLOCK),      \
        [UNITS_IN_BLOCK-1] = PHYSICAL_BLOCKS*AVG_SPARE_IN_BLOCK,    \
        /*[UNITS_IN_BLOCK-2] = PHYSICAL_BLOCKS*(AVG_SPARE_IN_BLOCK-1),*/ \
    }


#define PERCENTAGE_DIV(x, y)     ((x) * 100 / (y))

#define MIN(x, y)  ((x) < (y) ? (x) : (y))

struct block_state {
    int blocks_by_used[UNITS_IN_BLOCK+1];
};

static unsigned get_random_unit_bucket_index(struct block_state *state)
{
    unsigned used_unit_index = random() % USED_UNITS;
    /* Weighted random to find some used unit */
    unsigned i;
    for(i = 1; i <= UNITS_IN_BLOCK; i++) {
        unsigned cur_block_used = state->blocks_by_used[i] * i;
        if(used_unit_index < cur_block_used) {
            return i;
        }
        used_unit_index -= cur_block_used;
    }
    assert(false);
}

static void print_state(
    struct block_state *state,
    unsigned long least_used_bucket_index,
    double avg_least_used_bucket_index)
{
    unsigned long i;

    printf("Block freedom histogram\n");
    unsigned jump = UNITS_IN_BLOCK / 8;
    for(i = 0; i <= UNITS_IN_BLOCK; i+=jump) {
        /* 5 = 12 minus 7 dashes */
        printf("%5s-------", "");
    }
    printf("\n");

    for(i = 0; i <= UNITS_IN_BLOCK; i+=jump) {
        /* 5 = 12 minus 3 minus 3 minus 1 dash */
        printf("%5s%3lu-%3lu", "", i, MIN(UNITS_IN_BLOCK, i+jump-1));
    }
    printf("\n");

    for(i = 0; i <= UNITS_IN_BLOCK; i+=jump) {
        unsigned total = 0;
        unsigned j;
        for(j = i; j <= MIN(UNITS_IN_BLOCK, i+jump-1); j++) {
            total += state->blocks_by_used[UNITS_IN_BLOCK-j];
        }
        /* 12 is the column width */
        printf(" %11d", total);
    }
    printf("\n");

    unsigned long free_in_least_used_bucket = UNITS_IN_BLOCK - least_used_bucket_index;
    printf("Most-free-block has %lu(%lu%%) free (%u blocks of that usage) (%lu used)\n",
           free_in_least_used_bucket,
           PERCENTAGE_DIV(free_in_least_used_bucket, UNITS_IN_BLOCK),
           state->blocks_by_used[least_used_bucket_index], least_used_bucket_index);
    printf("Average most-free-block so far: %g\n", UNITS_IN_BLOCK - avg_least_used_bucket_index);
}

static unsigned long compute_least_used(struct block_state *state)
{
    unsigned long i;
    for(i = 0; i <= UNITS_IN_BLOCK; i++) {
        if(state->blocks_by_used[i] > 0) {
            return i;
        }
    }
    assert(0); // no buckets exist!
}

int main()
{
    /* Initialize where whole address space is used optimally (spare
     * free blocks, all other capacity in fully used blocks) */
    struct block_state state = {
        .blocks_by_used = INITIAL_BLOCKS_BY_USED,
    };

#define PRINT_VAR(name) printf(#name " = %lu\n", name)

    PRINT_VAR(UNITS_IN_BLOCK);
    PRINT_VAR(PHYSICAL_BLOCKS);
    PRINT_VAR(USED_UNITS);

    /* printf("Spare is %lu%% of physical address space (or %lu%% of logical)\n", */
    /*        (unsigned long)(100ul * SPARE_RATIO), */
    /*        PERCENTAGE_DIV(SPARE_PHYSICAL_IN_BLOCKS, USED_PHYSICAL_IN_BLOCKS)); */

    srandom(0);
    unsigned long total_least_used_bucket_index = 0;
    unsigned long used_bucket_index_sample_count = 0;

    unsigned long iter;
    for(iter = 0; ITER_STOP(iter); iter++) {
        unsigned least_used_bucket_index = compute_least_used(&state);
        if(COUNT_LEAST_USED_BUCKET_INDEX(iter)) {
            used_bucket_index_sample_count++;
            total_least_used_bucket_index += least_used_bucket_index;
        }
        if(0 == iter % PRINT_ITERATION_INTERVAL) {
            print_state(&state, least_used_bucket_index,
                        total_least_used_bucket_index * 1.0 / used_bucket_index_sample_count);
        }

        unsigned smallest_size;
        for(smallest_size = 0; ; smallest_size++) {
            assert(smallest_size < UNITS_IN_BLOCK);
            if(state.blocks_by_used[smallest_size]) break;
        }

        state.blocks_by_used[smallest_size]--;
        state.blocks_by_used[UNITS_IN_BLOCK]++;
        unsigned new_written_units = UNITS_IN_BLOCK - smallest_size;

        /* Wrote new_written_units on the most-free-block, which
         * invalidates blocks from all the existing units */
        unsigned i;
        for(i = 0; i < new_written_units; i++) {
            unsigned bucket_index = get_random_unit_bucket_index(&state);
            assert(bucket_index >= 1);
            assert(bucket_index <= UNITS_IN_BLOCK);
            state.blocks_by_used[bucket_index]--;
            state.blocks_by_used[bucket_index-1]++;
        }
    }
    return 0;
}
