
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
    mutex* m;
    size_t maxpoolsize;  // max size of one pool in items

    int poolcount;  // amount of "sub" pools we have
    struct pool* pools;  // pointer to all pools
    int firstfreepool; // pool with lowest index which is not full
};

struct poolAllocator* poolAllocator_New(size_t size, int threadsafe) {
    if (size == 0) {
        // we don't support allocating nothing.
        return NULL;
    }
    struct poolAllocator* p = malloc(sizeof(*p));
    if (!p) {
        return NULL;
    }
    memset(p, 0, sizeof(*p));
    p->size = size;
    if (threadsafe) {
        // for a thread-safe pool allocator,
        // use a mutex:
        p->m = mutex_Create();
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

void* poolAllocator_Alloc(struct poolAllocator* p) {
    if (p->m) {
        mutex_Lock(p->m);
    }

    // first, see if there is an existing pool with free space:
    int usepool = -1;
    usepool = p->firstfreepool;  // free pool we remembered
    if (usepool < 0) {
        // find first non-full pool:
        int i = 0;
        while (i < p->poolcount) {
            if (p->pools[i].filled < p->maxpoolsize) {
                p->firstfreepool = i;
                usepool = i;
                break;
            }
            i++;
        }
    }
    int expandpool = 0;
    if (usepool < 0) {
        // find first pool that can be expanded:
        int i = 0;
        while (i < p->poolcount) {
            if (p->pools[i].size < p->maxpoolsize) {
                usepool = i;
                expandpool = 1;
                break;
            }
            i++;
        }
    }
    if (usepool < 0) {
        // we need to add a completely new pool!
        // let's do this... leerooooy jj

        // first, resize the pool array:
        int newcount = p->poolcount + 1;
        void* newpools = realloc(p->pools, sizeof(void*) * newcount);
        if (!newpools) {
            if (p->m) {
                mutex_Release(p->m);
            }
            return NULL;
        }
        p->pools = newpools;

        // initialise new empty pool:
        struct pool* newp = &(p->pools[p->poolcount]);
        memset(newp, 0, sizeof(*newp));
        newp->firstfreeslot = 1;

        // now use this pool for our allocation:
        usepool = p->poolcount;
        expandpool = 1;  // it needs to be expanded from size 0

        // remember we have a new pool now:
        p->poolcount++;
    }
    if (expandpool) {
        // need to expand this pool first before using it.
        struct pool* pool = &(p->pools[usepool]);
        int newsize = pool->size * 2;
        if (newsize == 0) {
            // use initial size of 4:
            newsize = 4;
        }
        // first, expand memory:
        void* newmem = realloc(pool->memory, sizeof(p->size) * newsize);
        if (!newmem) {
            // whoops, out of memory?
            if (p->m) {
                mutex_Release(p->m);
            }
            return NULL;
        }
        // now expand filled slots info array:
        void* newarray = realloc(pool->filledslots, sizeof(slot_t) * newsize);
        if (!newarray) {
            // out of memory apparently!
            free(newmem);
            if (p->m) {
                mutex_Release(p->m);
            }
            return NULL;
        }
        // set new pointers to larger areas:
        pool->filledslots = newarray;
        pool->memory = newmem;
        // set new free slot hint:
        if (pool->firstfreeslot < 0) {
            pool->firstfreeslot = pool->size;
        }
        // mark new slots as empty:
        memset(pool->filledslots + sizeof(slot_t) * pool->size, 0,
        sizeof(slot_t) * (newsize - pool->size));
        // update new size:
        pool->size = newsize;
    }
    // see if we have a nice free slot:
    struct pool* pool = &(p->pools[usepool]);
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
            "no free slot found. This indicates memory corruption or a bug.");
            exit(1);
            return NULL;
        }
    }
    // use the free slot and return memory address:
    pool->filledslots[slot] = 1;
    void* ptr = ((char*)pool->memory) + slot * (p->size);
    pool->firstfreeslot = -1;
    if (slot + 1 < (int)pool->size && pool->filledslots[slot+1] == 0) {
        pool->firstfreeslot = slot + 1;
    } 

    if (p->m) {
        mutex_Release(p->m);
    }
    return ptr;
}

void poolAllocator_Free(struct poolAllocator* p, void* memp) {

}

