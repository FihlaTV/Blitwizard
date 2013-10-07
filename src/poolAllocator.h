
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

#ifndef BLITWIZARD_POOLALLOCATOR_H_
#define BLITWIZARD_POOLALLOCATOR_H_

// This implements a memory pool for unordered items
// of same size. The pool allocation is supposed to
// speed up things as opposed to single malloc() calls.

// If you want to use standard malloc instead, uncomment this:
//#define DISABLE_POOL_ALLOC

#include <unistd.h>
#include <stdlib.h>

struct poolAllocator;

// Set up a new pool allocator. Thread-safe pool allocators
// allow thread-safe usage of poolAllocator_Alloc from many
// threads at once, but they are naturally a bit slower.
struct poolAllocator* poolAllocator_create(size_t size, int threadsafe);

// Allocate using the given allocator. Please note thread-safety
// depends on whether the allocator was set up with thread-safety enabled.
// For allocators which aren't thread-safe, don't call this function
// at the same time from multiple different threads.
//
// The allocation always has the size specified when setting up
// the allocator.
void* poolAllocator_alloc(struct poolAllocator* p);

// Free memory allocated by a specific pool allocator.
void poolAllocator_free(struct poolAllocator* p, void* memptr);

// Destroy a pool allocator and free all memory used by it.
// Do not use memory allocated by it afterwards, and don't
// attempt to use poolAllocator_Free on any of it.
void poolAllocator_destroy(struct poolAllocator* p);

#endif  // BLITWIZARD_POOLALLOCATOR_H_

