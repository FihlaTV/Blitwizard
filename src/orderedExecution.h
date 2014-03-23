
/* blitwizard game engine - source code file

  Copyright (C) 2013-2014 Jonas Thiem

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

#ifndef BLITWIZARD_ORDEREDEXECUTION_H_
#define BLITWIZARD_ORDEREDEXECUTION_H_

// This implements an ordered execution pipeline.
// (api is not thread-safe!)
//
// Basically, a pipeline consists of one function,
// and many data pointers with order dependencies.
// 
// Each data pointer is supposed to represent a to-be-visited
// object the visitor function needs to look at,
// and the order dependencies allow setting up a required
// order in which the objects need to be looked at/visited.
//
// The idea of the whole pipeline is that dependencies
// are only specified per object, and dependency resolution
// into a final order is completely done for you.
//
// Basic usage:
//
//  First, create a pipeline with orderedExecution_New
//  with the function that looks at the objects.
//  Then, set up all the objects with orderedExecution_Add
//  and once you're done, use orderedExecution_Do to
//  make the function look at all objects in the resolved
//  order as often as you want.
//
// Error checking and performance:
//
//  You can repeatedly use orderedExecution_Add or
//  orderedExecution_Remove at any later point.
//  Those functions are designed to be fast, therefore
//  they don't do much error checking.
//
//  Conflicting order dependencies will be reported by
//  orderedExecution_Do, which gives a list of data
//  pointers for which the dependencies would need
//  changing to resolve conflicts.

// struct which represents a whole pipeline:
// (manipulation functions are below)
struct orderedExecutionPipeline;

// struct which represents the order dependencies
// of a specific entry:
struct orderedExecutionOrderDependencies {
    // dependency lists:
    void **before; // list of data pointers which need to run before
    size_t beforeEntryCount;
    void **after;  // list of data pointers which need to run after
    size_t afterEntryCount;
    // Note: listing data pointers which haven't been
    // added yet with orderedExeuction_Add is valid.

    // special groups:
    int runAfterAll;  // a special group running after all normal entries
    int runBeforeAll;  // a special group running before all normal entries
};

// create a new pipeline:
struct orderedExecutionPipeline *orderedExecution_new(void (*func)(void *data));

// add a new entry to be called (represented by the data entry):
int orderedExecution_add(struct orderedExecutionPipeline *pipeline,
void *data, struct orderedExecutionOrderDependencies *deps);
// This function will make a deep copy of the deps struct:
// You may free the dependency struct (including the before/after list)
// after passing it to this function.
// Returns 1 on success. Returns 0 in out of memory conditions.
// The deps are NOT tested for validity at this stage. This happens
// when orderedExecution_Do is called.

// remove an entry again:
void orderedExecution_remove(struct orderedExecutionPipeline *pipeline,
void *data);

// call all entries in the pipeline in their according order:
void orderedExecution_do(struct orderedExecutionPipeline *pipeline,
void **datawithfaultydependencies);

#endif  // BLITWIZARD_ORDEREDEXECUTION_H_

