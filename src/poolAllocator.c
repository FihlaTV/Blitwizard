
/* blitwizard game engine - source code file

  Copyright (C) 2013 Jonas Thiem

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "threading.h"
#include "poolAllocator.h"

#define slot_t int32_t

struct pool {
    void* memory;  // memory area
    slot_t* filledslots;  // list of free and occupied items
    size_t size;  // size in items
    size_t filled;  // amount of filled (occupied) items
    int firstfreeslot;
};

struct poolAllocator {
    size_t size;  // the size of one item which can be allocated
    mutex* m;  // mutex if thread-safe pool, otherwise NULL
    size_t maxpoolsize;  // max size of one "sub" pool in items
    size_t nextpoolsize;  // recommended size for next "sub" pool

    int poolcount;  // amount of "sub" pools we have
    struct pool* pools;  // pointer to all pools
    int firstfreepool; // pool with lowest index which is not full
};

struct poolAllocator* poolAllocator_create(size_t size, int threadsafe) {
    if (size == 0) {
        // we don't support allocating nothing.
        return NULL;
    }
    struct poolAllocator* p = malloc(sizeof(*p));
    if (!p) {
        return NULL;
    }
    memset(p, 0, sizeof(*p));
    p->nextpoolsize = 4;
    p->size = size;
    if (threadsafe) {
        // for a thread-safe pool allocator,
        // use a mutex:
        p->m = mutex_create();
        if (!p->m) {
            free(p);
            return NULL;
        }
    }
    p->firstfreepool = -1;

    // to avoid requiring very lengthy memory areas,
    // we don't want a single sub-pool to exceed roughly 32M
    p->maxpoolsize = (32 * 1024 * 1024) / size;
    return p;
}

void* poolAllocator_alloc(struct poolAllocator* p) {
#ifdef DISABLE_POOL_ALLOC
    return malloc(p->size);
#else

    if (p->m) {
        mutex_lock(p->m);
    }

    // first, see if there is an existing pool with free space:
    int usepool = -1;
    int expandpool = 0;
    usepool = p->firstfreepool;  // free pool we remembered
    if (usepool < 0) {
        // find first non-full pool, or if none the first zero size:
        int zerosizeid = -1;
        int i = 0;
        while (i < p->poolcount) {
            if (p->pools[i].filled < p->pools[i].size) {
                p->firstfreepool = i;
                usepool = i;
                break;
            } else {
                if (p->pools[i].size == 0) {
                    zerosizeid = i;
                }
            }
            i++;
        }
        if (usepool < 0 && zerosizeid >= 0) {
            usepool = zerosizeid;
            expandpool = 1;
        }
    }
    if (usepool < 0) {
        // we need to add a completely new pool!
        // let's do this... leerooooy jj

        // first, resize the pool array:
        int newcount = p->poolcount + 1;
        void* newpools = realloc(p->pools, sizeof(*p->pools) * newcount);
        if (!newpools) {
            if (p->m) {
                mutex_release(p->m);
            }
            return NULL;
        }
        p->pools = newpools;

        // initialise new empty pool:
        struct pool* newp = &(p->pools[p->poolcount]);
        memset(newp, 0, sizeof(*newp));
        newp->firstfreeslot = -1;

        // now use this pool for our allocation:
        usepool = p->poolcount;

        // remember we have a new pool now:
        p->poolcount++;
        expandpool = 1;
    }
    if (expandpool) {
        // need to allocate the pool data now, see how much we want:
        struct pool* pool = &(p->pools[usepool]);
        int newsize = p->nextpoolsize;
        p->nextpoolsize = newsize * 2;
        if (p->nextpoolsize > p->maxpoolsize) {
            p->nextpoolsize = p->maxpoolsize;
        }

        // allocate pool data:
        void* newmem = malloc(p->size * newsize);
        if (!newmem) {
            // whoops, out of memory?
            if (p->m) {
                mutex_release(p->m);
            }
            return NULL;
        }

        // now expand filled slots info array:
        void* newarray = malloc(sizeof(slot_t) * newsize);
        if (!newarray) {
            // out of memory apparently!
            free(newmem);
            if (p->m) {
                mutex_release(p->m);
            }
            return NULL;
        }
        // set pointers to allocated stuff:
        pool->filledslots = newarray;
        pool->memory = newmem;
        // set free slot hint:
        if (pool->firstfreeslot < 0) {
            pool->firstfreeslot = 0;
        }
        // mark slots as empty:
        memset(pool->filledslots, 0, sizeof(slot_t) * newsize);
        // store size:
        pool->size = newsize;
    }
    // see if we have a nice free slot:
    struct pool* pool = &(p->pools[usepool]);
    assert(pool->filled < pool->size);
    assert(pool->memory || pool->filled == 0);
    int slot = pool->firstfreeslot;
    if (slot < 0) {
        unsigned int i = 0;
        while (i < pool->size) {
            if (!pool->filledslots[i]) {
                pool->firstfreeslot = i;
                slot = i;
                break;
            }
            i++;
        }
        if (slot < 0) {
            fprintf(stderr, "FATAL POOL ALLOCATION ERROR - "
            "no free slot found. This indicates memory corruption or a bug.\n");
            fprintf(stderr, "Program abort.\n");
            exit(1);
            return NULL;
        }
    }

    // let's do some asserts at this point:
    assert(pool->filled < pool->size);
    assert(slot >= 0 && slot < (int)pool->size);
    assert(pool->filledslots[slot] == 0);

    // use the free slot and return memory address:
    pool->filledslots[slot] = 1;
    pool->filled++;

    // update firstfreepool id:
    if (pool->filled >= pool->size) {
        if (usepool + 1 < p->poolcount && p->pools[usepool + 1].filled <
        p->pools[usepool + 1].size) {
            p->firstfreepool = usepool + 1;
        } else {
            p->firstfreepool = -1;
        }
    } else {
        p->firstfreepool = usepool;
    }

    void* ptr = (char*)((char*)pool->memory) + slot * (p->size);
    // printf("Ptr: %p, pool memory: %p, offset: %d\n", ptr,
    // pool->memory, ptr - pool->memory);
    pool->firstfreeslot = -1;
    if (slot + 1 < (int)pool->size && pool->filledslots[slot+1] == 0) {
        pool->firstfreeslot = slot + 1;
    } 

    if (p->m) {
        mutex_release(p->m);
    }
    // printf("Handing out #%d %d/%u\n", usepool, slot,
    // (unsigned int)pool->size);
    return ptr;
#endif
}

void poolAllocator_free(struct poolAllocator* p, void* memp) {
#ifdef DISABLE_POOL_ALLOC
    free(memp);
#else
    if (!memp) {return;}
    if (p->m) {
        mutex_lock(p->m);
    }

    // find the pool this is in, seeking backwards:
    int i = p->poolcount - 1;
    while (i >= 0) {
        if (memp >= p->pools[i].memory && (char*)memp <
        (char*)p->pools[i].memory + p->size * p->pools[i].size) {
            // this is what it belongs to!
            int offset = memp - p->pools[i].memory;
            int slot = (offset / p->size);
            assert(slot >= 0 && slot < (int)p->pools[i].size);
            assert(p->pools[i].filledslots[slot] == 1);
            p->pools[i].filledslots[slot] = 0;
            p->pools[i].filled--;
            if (p->pools[i].filled == 0) {
                // wipe pool empty to regain memory:
                free(p->pools[i].memory);
                p->pools[i].memory = NULL;
                free(p->pools[i].filledslots);
                p->pools[i].filledslots = NULL;
                p->pools[i].size = 0;
                p->pools[i].firstfreeslot = -1;
                if (p->firstfreepool == i) {
                    p->firstfreepool = -1;
                }
            } else {
                // update first free pool
                if (i < p->firstfreepool) {
                    p->firstfreepool = i;
                }
            }
            if (p->m) {
                mutex_release(p->m);
            }
            return;
        }
        i--;
    }
    if (p->m) {
        mutex_release(p->m);
    }
    fprintf(stderr, "*******\n");
    fprintf(stderr, "Invalid poolallocator_free on allocator %p, "
    "pointer %p: address not found in pool array\n", p, memp);
    assert(1 == 2);
    exit(1);
#endif
}

