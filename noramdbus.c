#include <stdio.h>
#include <stdlib.h>             /* random */
#include <stdbool.h>
#include <assert.h>

#define BUCKET_COUNT     (1ULL<<23)
#define RAM_BUCKET_COUNT (BUCKET_COUNT*2)
#define SIZE_COUNT       4096

unsigned weighted_random(unsigned weights[], unsigned count, unsigned long sum)
{
    assert(sum < 1ULL << 32);
    unsigned r = ((unsigned)rand()) % sum;
    for(unsigned i = 0; i < count; i++)
    {
        if(r < weights[i]) {
            return i;
        }
        r -= weights[i];
    }
    assert(false);
}

int main()
{
    unsigned ram_free = RAM_BUCKET_COUNT;
    unsigned by_size[SIZE_COUNT] = {0};
    by_size[0] = BUCKET_COUNT;
    unsigned last_non_zero = 0;

    unsigned long misses = 0;
    unsigned long total_size_to_flush = 0;
    for(unsigned long dbus = 0; ; dbus++) {
        /* Weighted random for a new DBU: */
        unsigned rand_bucket_size =
            weighted_random(by_size, SIZE_COUNT, BUCKET_COUNT);

        assert(by_size[rand_bucket_size] > 0);
        --by_size[rand_bucket_size];
        assert(rand_bucket_size < SIZE_COUNT-1);
        ++by_size[rand_bucket_size+1];
        last_non_zero =
            last_non_zero < rand_bucket_size+1
            ? rand_bucket_size+1
            : last_non_zero;

        ram_free--;
        if(0 == ram_free) {
            /* flush a random bucket: */
            unsigned size_to_flush =
#if 0
                1 + weighted_random(
                    by_size+1, SIZE_COUNT-1,
                    BUCKET_COUNT - by_size[0]);
#else
            last_non_zero;
#endif
            assert(by_size[size_to_flush]);
            --by_size[size_to_flush];
            if(size_to_flush == last_non_zero) {
                while(0 == by_size[last_non_zero]) {
                    assert(last_non_zero > 0);
                    last_non_zero--;
                }
            }
            ++by_size[0];
            ram_free += size_to_flush;

            misses++;
            total_size_to_flush += size_to_flush;
        }

        if(0 == dbus % (1ULL << 22)) {
            printf("RAM free: %u\n", ram_free);
            printf("DBU count: %lu\n", dbus);
            printf("Flushed %lu times\n", misses);
            printf("Average DBUs flushed each time: %lf\n",
                   total_size_to_flush * 1.0d / misses);
            printf("DBUS / misses: %lf\n",
                   dbus * 1.0d / misses);
        }
    }
    return 0;
}
