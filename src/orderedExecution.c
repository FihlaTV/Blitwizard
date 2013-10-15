
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

#include "poolAllocator.h"
#include "orderedExecution.h"

// this represents an entry to be called,
// with the according deps:
struct orderedExecutionEntry {
    struct orderedExecutionOrderDependencies deps;
    void* data;
    struct orderedExecutionEntry* prev,* next;
};

struct orderedExecutionEntryHashBucket {
    void** othersWantThisAfterThem;
    void** othersWantThisBeforeTHem;
    struct orderedExecutionEntryHashBucket* next,*prev;
};

#define HASHTABLESIZE 1024

// the execution pipeline with all entries:
struct orderedExecutionPipeline {
    // those are still completely unsorted:
    struct orderedExecutionEntry* newUnorderedEntries;

    // those are nicely sorted already:
    struct orderedExecutionEntry* orderedListBeforeAll;
    struct orderedExecutionEntry* orderedListBeforeAllEnd;
    struct orderedExecutionEntry* orderedListNormal;
    struct orderedExecutionEntry* orderedListNormalEnd;
    struct orderedExecutionEntry* orderedListAfterAll;
    struct orderedExecutionEntry* orderedListAfterAllEnd;
    struct orderedExecutionEntry* orderedListInvalid;

    // the pool allocator we use:
    struct poolAllocator* allocator;

    // hash table for data pointer -> orderedExecutionEntry
    struct orderedExecutionEntryHashBucket* table[HASHTABLESIZE];
};

struct orderedExecutionPipeline* orderedExecution_new(
void (*func)(void* data)) {
    struct orderedExecutionPipeline* pi = malloc(sizeof(*pi));
    if (!pi) {
        return NULL;
    }
    memset(pi, 0, sizeof(*pi));
    pi->allocator = poolAllocator_create(
        sizeof(struct orderedExecutionEntry), 0);
    if (!pi->allocator) {
        free(pi);
        return NULL;
    }
    return pi;
}

struct orderedExecutionEntryHashBucket* orderedExecution_getOrCreateBucket(
struct orderedExecutionPipeline* p, void* data) {

}

int orderedExecution_add(struct orderedExecutionPipeline* p, void* data,
struct orderedExecutionOrderDependencies* deps) {
    // new ordered execution entry struct:
    struct orderedExecutionEntry* e = poolAllocator_alloc(p->allocator);
    if (!e) {
        return 0;
    }
    memset(e, 0, sizeof(*e));
    e->data = data;
    // deep copy of deps:
    memcpy(&e->deps, deps, sizeof(*deps));
    if (e->deps.beforeEntryCount > 0) {
        e->deps.before = malloc(sizeof(void*) * e->deps.beforeEntryCount);
        if (!e->deps.before) {
            free(e);
            return 0;
        }
    }
    if (e->deps.afterEntryCount > 0) {
        e->deps.after = malloc(sizeof(void*) * e->deps.afterEntryCount);
        if (!e->deps.after) {
            free(e->deps.before);
            free(e);
            return 0;
        }
    }
    memcpy(e->deps.before, deps->before,
    sizeof(void*) * e->deps.beforeEntryCount);
    memcpy(e->deps.after, deps->after,
    sizeof(void*) * e->deps.afterEntryCount);
    // now let's wee what we want to do with this entry:

    // obvious invalid combination:
    if (e->deps.runAfterAll && e->deps.runBeforeAll) {
        // add to invalid list:
        if (p->orderedListInvalid) {
            e->next = p->orderedListInvalid;
            p->orderedListInvalid->prev = e;
            p->orderedListInvalid = e;
        } else {
            p->orderedListInvalid = e;
        }
        return 1;
    }

    // if it doesn't have any deps, this is very easy:
    if (e->deps.afterEntryCount == 0 && e->deps.beforeEntryCount == 0) {
        // ... if nothing wants this before or after it:
        struct orderedExecutionEntryHashBucket* hb = 
        orderedExecution_getOrCreateBucket(p, data);
    }
    return 1;
}

void orderedExecution_remove(struct orderedExecutionPipeline* p, void* data) {

}

void orderedExecution_do(struct orderedExecutionPipeline* p, void** faulty) {

}


