#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>

#define UNITS_IN_BLOCK           128lu
#define PHYSICAL_BLOCKS          1000000lu /* total */
#define SPARE_RATIO              0.375 /* Make sure {UNITS_IN_BLOCK, PHYSICAL_BLOCKS}*SPARE_RATIO are whole numbers */
#define SPARE_PHYSICAL_IN_BLOCKS ((unsigned long)(SPARE_RATIO * PHYSICAL_BLOCKS))
#define USED_PHYSICAL_IN_BLOCKS  (PHYSICAL_BLOCKS - SPARE_PHYSICAL_IN_BLOCKS)
#define USED_UNITS               (UNITS_IN_BLOCK * USED_PHYSICAL_IN_BLOCKS)

#define INITIAL_BLOCKS_BY_USED                                          \
    {                                                                   \
        /*[UNITS_IN_BLOCK - (unsigned long)(UNITS_IN_BLOCK*SPARE_RATIO)] = PHYSICAL_BLOCKS,*/ \
        [0] = SPARE_PHYSICAL_IN_BLOCKS,                                 \
        [UNITS_IN_BLOCK] = USED_PHYSICAL_IN_BLOCKS,                     \
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

static void print_state(struct block_state *state)
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

    for(i = 0; i <= UNITS_IN_BLOCK; i++) {
        if(state->blocks_by_used[i] > 0) {
            printf("Most-free-block has %lu(%lu%%) free (%u blocks of that usage) (%lu used)\n",
                   UNITS_IN_BLOCK-i, PERCENTAGE_DIV(UNITS_IN_BLOCK-i, UNITS_IN_BLOCK),
                   state->blocks_by_used[i], i);
            break;
        }
    }
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

    printf("Spare is %lu%% of physical address space (or %lu%% of logical)\n",
           (unsigned long)(100ul * SPARE_RATIO),
           PERCENTAGE_DIV(SPARE_PHYSICAL_IN_BLOCKS, USED_PHYSICAL_IN_BLOCKS));

    srandom(0);
    unsigned iter;
    for(iter = 0; ; iter++) {
        if(0 == iter % (1<<17)) print_state(&state);

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
