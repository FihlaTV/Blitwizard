
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

#include <string.h>

#include "threading.h"
#include "poolAllocator.h"

struct poolAllocator {
    size_t size;
    mutex* m;
    size_t maxpoolsize;

    int poolcount;
    void* pools;
    size_t* poolsize;
    int firstinterestingpool; // pool not at max size with lowest index
};

struct poolAllocator* poolAllocator_New(size_t size, int threadsafe) {
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
    p->firstinterestingpool = -1;
    return p;
}

void* poolAllocator_Alloc(struct poolAllocator* p) {
    if (p->m) {
        mutex_Lock(p->m);
    }
    
    if (p->m) {
        mutex_Release(p->m);
    }
}

