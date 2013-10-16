
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

#include <stdint.h>
#include <string.h>

#include "poolAllocator.h"
#include "orderedExecution.h"

// this represents an entry to be called,
// with the according deps:
struct orderedExecutionEntry {
    struct orderedExecutionOrderDependencies deps;
    void* data;
    int list; // -1: beforeAll list, 0: normal, 1: afterAll
    struct orderedExecutionEntry* prev,* next;
};

struct orderedExecutionEntryHashBucket {
    int scheduledForRemoval;
    void** othersWantThisAfterThem;
    size_t othersWantThisAfterThemCount;
    void** othersWantThisBeforeThem;
    size_t othersWantThisBeforeThemCount;
    void* data;  // the call data ptr
    struct orderedExecutionEntry* listEntry;
    struct orderedExecutionEntryHashBucket* next,* prev;
};

#define HASHTABLESIZE 1024

// the execution pipeline with all entries:
struct orderedExecutionPipeline {
    // those are scheduled for removal or addition:
    struct orderedExecutionEntry** entriesToBeAdded;
    size_t entriesToBeAddedCount;
    size_t entriesToBeAddedAllocation;
    void** entriesToBeRemoved;
    size_t entriesToBeRemovedCount;
    size_t entriesToBeRemovedAllocation;

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

static int orderedExecution_scheduleEntryForAddRemove(
void*** list, size_t* listLength, size_t* listAllocation,
void* data) {
    // first, ensure list is large enough:
    if (!*list || *listAllocation == 0) {
        int i = 8;  // initial list size
        void** newp = realloc(*list, sizeof(void*) * i);
        if (!newp) {
            return 0;
        }
        *list = newp;
        *listAllocation = 0;
    } else {
        if (*listAllocation <= *listLength) {
            // double the allocation:
            int newAllocation = (*listAllocation) * 2;
            void** newp = realloc(*list, sizeof(void*) * newAllocation);
            if (!newp) {
                return 0;
            }
            *list = newp;
            *listAllocation = newAllocation;
        }
    }
    // now add entry:
    int i = (*listLength);
    *listLength = (*listLength)+1;
    (*list)[i] = data;
    return 1;
}

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

uint32_t hashfunc(void* data, uint32_t max) {
    uint64_t dataint = (uint64_t)data;
    return (uint32_t)((uint64_t)dataint % (uint64_t)max);
}

struct orderedExecutionEntryHashBucket* orderedExecution_getOrCreateBucket(
struct orderedExecutionPipeline* p, void* data, int create) {
    // get or create the hash bucket for the given data ptr
    uint32_t slot = hashfunc(data, HASHTABLESIZE);
    // see if it already exists:
    struct orderedExecutionEntryHashBucket* hb = p->table[slot];
    while (hb) {
        if (hb->data == data) {
            // we found it!
            return hb;
        }
        hb = hb->next;
    }
    // if we aren't supposed to creat things, quit here
    if (!create) {
        return NULL;
    }
    // we need to add it! allocate fresh bucket entry..
    hb = malloc(sizeof(*hb));
    if (!hb) {
        return NULL;
    }
    memset(hb, 0, sizeof(*hb));
    hb->data = data;
    if (p->table[slot]) {  // prepend to list
        hb->next = p->table[slot];
        p->table[slot]->prev = hb;
    }
    p->table[slot] = hb;
    return data;  // return bucket
}

void orderedExecution_deleteBucket(struct orderedExecutionPipeline* p,
void* data) {
    // find the hash bucket and delete it, if found
    uint32_t slot = hashfunc(data, HASHTABLESIZE);
    struct orderedExecutionEntryHashBucket* hb = p->table[slot];
    while (hb) {
        if (hb->data == data) {
            // this is the entry we would like to remove
            if (hb->next) {
                hb->next->prev = hb->prev;
            }   
            if (hb->prev) {
                hb->prev->next = hb;
            } else {
                p->table[slot] = hb->next;
            }
            free(hb);
            return;
        }
        hb = hb->next;
    }
}

// add an execution entry into a given linked list:
static void orderedExecution_addToList(struct orderedExecutionEntry** start,
struct orderedExecutionEntry* prev, struct orderedExecutionEntry* entry,
struct orderedExecutionEntry* next, struct orderedExecutionEntry** end) {
    if (!*start || !*end) {
        *start = entry;
        *end = entry;
    } else if (!prev) {
        entry->next = *start;
        (*start)->prev = entry;
        *start = entry;
    } else if (!next) {
        entry->prev = *end;
        (*end)->next = entry;
        *end = entry;
    }
}

int orderedExecution_add(struct orderedExecutionPipeline* p,
void* data, struct orderedExecutionOrderDependencies* deps) {
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

    return orderedExecution_scheduleEntryForAddRemove(
    (void***)&p->entriesToBeAdded, &p->entriesToBeAddedCount,
    &p->entriesToBeAddedAllocation, e);
}

static int orderedExecution_add_internal(struct orderedExecutionPipeline* p,
struct orderedExecutionEntry* e) {
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
        orderedExecution_getOrCreateBucket(p, e->data, 1);
        hb->data = e->data;
        hb->listEntry = e;

        // check for before/after requirements on this entry:
        if (hb->othersWantThisAfterThemCount == 0 &&
        hb->othersWantThisBeforeThemCount == 0) {
            // just add it at the end of the list:
            if (e->deps.runAfterAll) {
                orderedExecution_addToList(&p->orderedListAfterAll,
                p->orderedListAfterAllEnd, e, NULL,
                &p->orderedListAfterAllEnd);
                e->list = 1;
            } else if (e->deps.runBeforeAll) {
                orderedExecution_addToList(&p->orderedListBeforeAll,
                p->orderedListBeforeAllEnd, e, NULL,
                &p->orderedListBeforeAllEnd);
                e->list = -1;
            } else {
                orderedExecution_addToList(&p->orderedListNormal,
                p->orderedListNormalEnd, e, NULL,
                &p->orderedListNormalEnd);
                e->list = 0;
            }
            return 1;
        }
    }

    // this is too complex to handle for us right now:
    if (p->orderedListInvalid) {
        e->next = p->orderedListInvalid;
        p->orderedListInvalid->prev = e;
    }
    p->orderedListInvalid = e;
    return 1;
}

void orderedExecution_remove(struct orderedExecutionPipeline* p,
void* data) {
    // schedule entry for removal:
    orderedExecution_scheduleEntryForAddRemove(
    &p->entriesToBeRemoved, &p->entriesToBeRemovedCount,
    &p->entriesToBeRemovedAllocation, data);

    // tell entry, if it exists, that it is about to be removed:
    struct orderedExecutionEntryHashBucket* hb =
    orderedExecution_getOrCreateBucket(p, data, 0);
    if (!hb || !hb->listEntry) {
        return;
    }
    hb->scheduledForRemoval = 1;
}

void orderedExecution_remove_internal(
struct orderedExecutionPipeline* p, void* data) {
    struct orderedExecutionEntryHashBucket* hb =
    orderedExecution_getOrCreateBucket(p, data, 0);
    if (!hb || !hb->listEntry) {
        return;
    }
    orderedExecution_deleteBucket(p, data);
}

void orderedExecution_do(struct orderedExecutionPipeline* p,
void** faulty) {
    // and and remove entries as scheduled:
    size_t i = 0;
    while (i < p->entriesToBeRemovedCount) {
        orderedExecution_remove_internal(p,
        p->entriesToBeRemoved[i]);
        i++;
    }
    p->entriesToBeRemovedCount = 0;
    i = 0;
    while (i < p->entriesToBeAddedCount) {
        int r = orderedExecution_add_internal(p,
        p->entriesToBeAdded[i]);
        i++;
    }
    p->entriesToBeAddedCount = 0;
}


