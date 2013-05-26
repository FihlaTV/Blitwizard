
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

#include "config.h"
#include "os.h"

#if (defined(USE_PHYSICS2D) || defined(USE_PHYSICS3D))

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "physics.h"
#include "physicsinternal.h"

struct cachedimpulse {
    double sourcex, sourcey, sourcez;
    double forcex, forcey, forcez;
};

struct cachedangularimpulse {
    union {
        double angle2d;
        struct {
            double x, y, z, r;
        };
    };
};

struct cachedphysicsobject {
    struct physicsobjectshape* shapes;
    int shapecount;
    
    struct physicsworld* world;
    
    void* userdata;
    
    int movable;
    
    int massisset;
    double mass;
    
    int masscenterisset;
    double masscenterx, masscentery, masscenterz;
    
    int gravityisset;
    double gravityx, gravity, gravityz;
    
    int rotationrestriction2d;
    int rotationrestriction3daxis;
    double rotationrestriction3dx, rotationrestriction3dy,
    rotationrestriction3dz;
    double rotationrestriction3dfull;
    
    int frictionisset;
    double friction;
    
    int angulardampingset;
    double angulardamping;
    
    int lineardampingset;
    double lineardamping;
    
    int restitutionset;
    double restitution;
    
    int positionset;
    double positionx, positiony, positionz;
    
    int rotation2dset;
    double rotation2d;
    
    int rotation3dset;
    double rotation3dx, rotation3dy, rotation3dz, rotation3dr;
    
    int velocityset;
    double velocityx, velocityy, velocityz;
    
    int angularvelocityset;
    double angularvelocity;
    
    int impulsecount;
    struct cachedimpulse* impulses;
    
    struct cachedphysicsobject* next;
};

struct cachedphysicsobject* cachedObjects = NULL;

int isInCallback = 0;

static struct cachedphysicsobject* physics_createCachedObjects(int count) {
    struct cachedphysicsobject* c_object;
    for (int i = 0; i < count; ++i) {
        c_object = (struct cachedphysicsobject*)malloc(sizeof(*c_object));
        memset (c_object, 0, sizeof(*c_object));
        c_object->next = cachedObjects;
        cachedObjects = c_object;
    }
    return c_object; // Return last element added, i.e. first element in list
}

static int physics_destroyCachedObject(struct cachedphysicsobject* c_object) {
    struct cachedphysicsobject *c, *c_prev;
    c = cachedObjects;
    c_prev = NULL;
    while (c != NULL) {
        if (c == c_object) {
            if (not c_prev) {
                cachedObjects = c->next;
            } else {
                c_prev->next = c->next;
            }
            free(c);
            return 0;
        }
        c_prev = c;
        c = c->next;
    }
    return 1;
}

static int physics_objectIsCached(void* object) {
    struct cachedphysicsobject* c = cachedObjects;
    while (c != NULL) {
        if ((void*)c == object) {
            return 1;
        }
        c = c->next;
    }
    return 0;
}

static struct physicsobject* physics_createObjectFromCache(struct
 cachedphysicsobject* c_object) {
    struct physicsobject* object;
    object = physics_createObject_internal(c_object->world, c_object->userdata,
     c_object->movable, c_object->shapes, c_object->shapecount);
    // TODO: something something
    return object;
}


/*
 Implementation of physics.h starts here
 */

struct physicsobject* physics_createObject(struct physicsworld* world,
 void* userdata, int movable, struct physicsobjectshape* shapelist,
 int shapecount) {
    if (isInCallback) {
        struct cachedphysicsobject* c_object = physics_createCachedObjects(1);
        c_object->world = world;
        c_object->userdata = userdata;
        c_object->movable = movable;
        c_object->shapes = shapelist;
        c_object->shapecount = shapecount;
        return (struct physicsobject*)c_object;
    } else {
        return physics_createObject_internal(world, userdata, movable,
         shapelist, shapecount);
    }
}

void physics_destroyObject(struct physicsobject* object) {
    struct cachedphysicsobject* c_object = (struct cachedphysicsobject*)object;
    // Relies on physics_destroyCachedObject() returning 1 for non-cached objs
    /* TODO: We can make this faster by checking for isInCallback instead if
     [cached objects exist] <=> [isInCallBack] holds.
     */
    if (physics_destroyCachedObject(c_object)) {
        physics_destroyObject_internal(object);
    }
}

#endif  // defined(USE_PHYSICS2D) || defined(USE_PHYSICS3D)
